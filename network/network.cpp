#include "network.h"

void Network::setupWifi()
{
    WiFi.begin(config->ssid, config->password);
    Debug.Info("Connecting to WiFi");
    int WaitCount = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
        WaitCount++;
        if (WaitCount > config.WifiConnectWaitLimit)
        {
            Debug.Warning("WiFi connect timeout");
            tft.println("WiFi connect timeout");
            sysState = SystemState::Error;
            while (1)
                ;
        }
    }
    Serial.print("\n");
    Debug.Debug("WiFi connected. IP address: " + WiFi.localIP().toString());
}

void Network::syncNTPTime()
{
    Debug.Info("Start syncing time");
    configTime(config.gmtOffset_sec, config.daylightOffset_sec, config->ntpServer);
    struct tm timeinfo;
    int ntpRetry = 0;
    while (!getLocalTime(&timeinfo) && ntpRetry < config.ntpRetryLimit)
    {
        Debug.Warning("NTP Sync Retry");
        delay(500);
        ntpRetry++;
    }
    if (getLocalTime(&timeinfo))
    {
        Debug.Info("NTP OK");
        rtc.setTimeStruct(timeinfo);
        Debug.Debug("synced time: ", true);
        Debug.Debug(rtc.getTime("%Y-%m-%d %H:%M:%S"));
    }
    else
    {
        Debug.Error("NTP sync faild");
        sysState = SystemState::Error;
        while (1)
            ;
    }
}

void Network::setupWebServer()
{

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    File file = openSDFile("/index.html");
    if (!file) {
      Debug.Warning("file not found.Request 500.");
      request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The index.html File in SD.");
      return;
    }
    file.close();
    request->send(SD, "/index.html", "text/html"); });

    server.on("/PlanEdit/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    File file = openSDFile("/PlanEdit.html");
    if (!file) {
      Debug.Warning("file not found.Request 500.");
      request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The PlanEdit.html File in SD.");
      return;
    }
    file.close();
    request->send(SD, "/PlanEdit.html", "text/html"); });

    server.on("/AJAX/planList", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    Debug.Info("Received a planList AJAX");
    const String tag = request->header("X-Requested-With");
    if (tag.isEmpty()) {
      Debug.Warning("AJAX does not have the head");
      request->send(403, "text/plain", "403 Forbidden!");
    } else {
      if (tag != "XMLHttpRequest") {
        Debug.Warning("AJAX does not have the right head");
        request->send(403, "text/plain", "403 Forbidden!");
      } else {
        File file = openSDFile("/plans.json");
        if (!file) {
          Debug.Warning("file not found.Request 500.");
          request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The plans.json File in SD.");
          return;
        }
        file.close();
        Debug.Info("AJAX is right.Send the request.");
        request->send(SD, "/plans.json", "json");
      }
    } });

    server.on(
        "/AJAX/planList",
        HTTP_POST,
        [](AsyncWebServerRequest *request)
        {
            request->send(200, "application/json", "{\"code\":200,\"massage\":\"ok\"}");
            if (request->_tempObject != nullptr)
            {
                free(((JsonRequestBody *)request->_tempObject)->buffer);
                delete (JsonRequestBody *)request->_tempObject;
                request->_tempObject = nullptr;
            }
        },
        nullptr,
        [](AsyncWebServerRequest *request,
           uint8_t *data, // ← 当前接收到的数据块指针
           size_t len,    // ← 当前块的长度
           size_t index,  // ← 当前块在整个body中的起始索引
           size_t total)
        {
            const String tag = request->header("X-Requested-With");
            if (tag != "XMLHttpRequest")
            {
                Debug.Warning("AJAX does not have the head");
                request->send(403, "text/plain", "403 Forbidden!");
                return;
            }
            if (index == 0)
            {
                auto *body = new JsonRequestBody();
                body->buffer = (uint8_t *)malloc(total + 1); // +1 用于结尾\0
                body->totalSize = total;
                body->receivedSize = 0;
                request->_tempObject = body;
            }

            // 获取当前请求关联的缓冲区
            auto *body = (JsonRequestBody *)request->_tempObject;
            if (body && body->buffer)
            {
                // 拼接当前数据块
                memcpy(body->buffer + index, data, len);
                body->receivedSize += len;
            }

            // 所有数据接收完毕，开始解析
            if (index + len == total)
            {
                if (body && body->buffer)
                {
                    body->buffer[total] = '\0'; // 确保字符串结束

                    if (!SD.exists("/plans.json"))
                    {
                        createJson();
                    }

                    DynamicJsonDocument doc(512);

                    DeserializationError error = deserializeJson(doc, body->buffer, body->totalSize + 1);

                    if (error)
                    {
                        Debug.Warning(String("解析失败：") + String(error.c_str()));
                        return;
                    }

                    File jsonFile = SD.open("/plans.json", FILE_READ);
                    if (!jsonFile)
                    {
                        Debug.Warning("file not open.Request 500.");
                        request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The plans.json File in SD.");
                        return;
                    }

                    size_t fileSize = jsonFile.size();

                    DynamicJsonDocument fileDoc(fileSize * 2);

                    DeserializationError error1 = deserializeJson(fileDoc, jsonFile);
                    jsonFile.close();
                    if (error1)
                    {
                        Debug.Warning("解析JSON文件错误");
                        request->send(500, "text/plain", "500 Server ERROR.Connot deserializeJson");
                        return;
                    }

                    insertIntoSortedJsonArray(fileDoc["planList"], doc, planListComparator);

                    jsonFile = SD.open("/plans.json", FILE_WRITE);
                    if (!jsonFile)
                    {
                        Debug.Warning("file not open.Request 500.");
                        request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The plans.json File in SD.");
                        return;
                    }

                    serializeJsonPretty(fileDoc, jsonFile);

                    jsonFile.close();
                    MenuChanged = true;
                    lastTimeStr = "";
                }
            }
        });

    server.on("/AJAX/planList", HTTP_DELETE, [](AsyncWebServerRequest *request)
              {
    const String tag = request->header("X-Requested-With");
    if (tag != "XMLHttpRequest") {
      Debug.Warning("AJAX does not have the head");
      request->send(403, "text/plain", "403 Forbidden!");
      return;
    }
    const String p = request->header("X-UUID-Data");
    if (p == "") {
      Debug.Warning("AJAX does not have uuid head");
      request->send(403, "text/plain", "403 Forbidden!");
      return;
    }
    Debug.Debug(p);
    File jsonFile = SD.open("/plans.json", FILE_READ);
    if (!jsonFile) {
      Debug.Warning("file not open.Request 500.");
      request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The plans.json File in SD.");
      return;
    }
    size_t fileSize = jsonFile.size();
    DynamicJsonDocument fileDoc(fileSize * 2);
    DeserializationError error1 = deserializeJson(fileDoc, jsonFile);
    jsonFile.close();
    if (error1) {
      Debug.Warning("解析JSON文件错误");
      request->send(500, "text/plain", "500 Server ERROR.Connot deserializeJson");
      return;
    }
    int targetIndex = -1;
    for (int i = 0; i < fileDoc["planList"].size(); i++) {
      String currentUuid = fileDoc["planList"][i]["id"].as<String>();
      Debug.Debug(currentUuid);
      if (currentUuid == p) {
        targetIndex = i;
        break;
      }
    }
    if (targetIndex != -1) {
      fileDoc["planList"].remove(targetIndex);
      jsonFile = SD.open("/plans.json", FILE_WRITE);
      if (!jsonFile) {
        Debug.Warning("file not open.Request 500.");
        request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The plans.json File in SD.");
        return;
      }
      serializeJsonPretty(fileDoc, jsonFile);
      jsonFile.close();
      MenuChanged = true;
      lastTimeStr = "";
    }
    request->send(200, "json", "{ code: 200, message: '计划删除成功' }"); });

    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      if (uploadFile) {
        uploadFile.close();
        uploadFile = File();
        request->send(200, "text/plain", "文件上传并更新成功！");
        Debug.Debug("文件上传完成！");
        preSortPlanList();
      } else {
        request->send(400, "text/plain", "文件上传失败！");
      } }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
              {
      if (!index) {
        Debug.Debug("开始上传文件：");
        Debug.Debug(filename);
        String filePath = "/" + filename;
        if (SD.exists(filename))
        {
          Debug.Debug("删除旧文件：");
          Debug.Debug(filename);
          if (!SD.remove(filename))
          {
            Debug.Debug("旧文件删除失败！");
          }
        }
        uploadFile = SD.open(filePath, FILE_WRITE);
        if (!uploadFile) {
          Debug.Warning("文件打开失败，无法写入！");
          return;
        }
      }

      if (len > 0 && uploadFile) {
        uploadFile.write(data, len);
        Debug.Debug("已接收：%u bytes\r", index + len);
      }

      if (final && uploadFile) {
        Debug.Debug("\n文件总大小：%u bytes\n", index + len);
      } });

    server.onNotFound([](AsyncWebServerRequest *request)
                      {
    Serial.printf("404错误 - 请求方法：%s，请求路径：%s\n",
                  request->methodToString(), request->url().c_str());
    request->send(404, "text/plain", "404 Not Found!"); });
    server.begin();
    Debug.Info("Web Server begin");
}
