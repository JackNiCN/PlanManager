#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <TFT_eSPI.h>
#include <SD.h>
#include <UTF8ToGB2312.h>
#include <WiFi.h>
#include <Debuger.h>
#include <ESP32Time.h>
#include <ESPAsyncWebServer.h>
#include <TFTMenu.h>
#include <ArduinoOTA.h>
const char* ssid = "vineky";
const char* password = "springhappyo621";
const int WifiConnectWaitLimit = 10;  // times of 500ms delay

const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;
const char* ntpServer = "ntp.aliyun.com";
const int ntpRetryLimit = 3;
unsigned long lastNtpSync = 0;
const unsigned long NTP_SYNC_INTERVAL = 3600000;  // 1 hour

int upButtonLastClickedTime = 0;
int downButtonLastClickedTime = 0;
int leftButtonLastClickedTime = 0;
int rightButtonLastClickedTime = 0;
int middleButtonLastClickedTime = 0;
int mutButtonLastClickedTime = 0;

TFT_eSPI tft;
ESP32Time rtc;
AsyncWebServer server(80);

enum SystemState {
  Initialization,
  Error,
  Screensave,
  Normal,
  Menu
};

struct JsonRequestBody {
  uint8_t* buffer = nullptr;
  size_t totalSize = 0;
  size_t receivedSize = 0;
};

void displayChinese(File& fontFile, int x, int y, String chStr, uint16_t color, bool noneBg = true, uint16_t bgcolor = TFT_BLACK);
void setupTFT();
void showSetupScreen();
void setupWifi();
void setupSD();
void syncNTPTime();
void setupWebServer();
void setupOTA();
void keyboardLoop();
File openSDFile(String fileName);
bool createJson();
bool drawMenuItem(SplitChinese::TextUnit* units, int count, int x, int y) {
  for (int i = 0; i < count; i++) {
    if (units[i].type == SplitChinese::CharType::TYPE_CHINESE) {
      File file = openSDFile("/HZK16");
      if (!file) {
        return false;
      }
      displayChinese(file, x, y, GB.from(units[i].content), TFT_WHITE, true);
      return true;
    }
  }
}
void setup() {
  Serial.begin(115200);
  delay(1000);
  Debug.Info("system start");
  setupTFT();
  setupSD();
  showSetupScreen();
  setupWifi();
  //setupOTA();
  syncNTPTime();
  setupWebServer();

  TFTMenu menu(&tft, 10);
  //menu.addItem(TFTMenuStruct::MenuItem(drawMenuItem,"hello"));
  menu.showMenu(1, 1, 160, 127, TFT_WHITE, TFT_BLACK);
  delay(1000);
}

void loop() {
  if (millis() - lastNtpSync > NTP_SYNC_INTERVAL) {
    syncNTPTime();
    lastNtpSync = millis();
  }
  keyboardLoop();
}


/**
 * 中文绘制函数
 * @param fontFile 字库文件（HZK16）
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param chStr GB2312编码的中文字符串
 * @param color 前景色
 * @param noneBg 是否不绘制背景色（true=不绘制背景色，false=绘制背景色）
 * @param bgcolor 背景色
 */
void displayChinese(File& fontFile, int x, int y, String chStr, uint16_t color, bool noneBg, uint16_t bgcolor) {
  int strLen = chStr.length();  // 字符串长度（每个中文占2字节，需成对处理）

  for (int i = 0; i < strLen; i += 2) {  // 每次取2字节（1个中文字符）
    // 1. 提取GB2312编码（区码=高字节，位码=低字节）
    uint8_t codeHigh = chStr[i];     // 区码（GB2312编码高8位）
    uint8_t codeLow = chStr[i + 1];  // 位码（GB2312编码低8位）

    // 2. 计算字库偏移量（16x16点阵占32字节，GB2312从0xA1A1开始）
    long offset = ((codeHigh - 0xA1) * 94 + (codeLow - 0xA1)) * 32;

    // 3. 从SD卡读取32字节点阵数据
    uint8_t dotMatrix[32];         // 存储16x16点阵（16行×2字节/行）
    fontFile.seek(offset);         // 移动文件指针到目标位置
    fontFile.read(dotMatrix, 32);  // 读取点阵数据
    // 4. 逐点渲染到ST7735屏幕
    for (int row = 0; row < 16; row++) {
      uint8_t dotHigh = dotMatrix[row * 2];     // 该行高8位像素数据
      uint8_t dotLow = dotMatrix[row * 2 + 1];  // 该行低8位像素数据

      for (int col = 0; col < 16; col++) {

        int charOffsetX = (i / 2) * 16;  // 字符横向偏移
        int charOffsetY = 0;             // 字符纵向偏移

        uint16_t pixelX = x + charOffsetX + col, pixelY = y + charOffsetY + row;

        // 跳过屏幕范围外的像素（避免越界）
        if (pixelX >= TFT_WIDTH || pixelY >= TFT_HEIGHT) continue;

        // 判断当前点是否点亮（1=亮，0=灭）
        bool isLight = false;
        if (col < 8) {  // 前8列对应高字节（从高位到低位）
          isLight = (dotHigh & (0x80 >> col)) != 0;
        } else {  // 后8列对应低字节
          isLight = (dotLow & (0x80 >> (col - 8))) != 0;
        }
        // 点亮像素（亮则画目标颜色，灭则画黑色背景）
        if (isLight) {
          tft.drawPixel(pixelX, pixelY, color);
        } else {
          if (!noneBg) {
            tft.drawPixel(pixelX, pixelY, bgcolor);
          }
        }
      }
    }
  }
}

void setupTFT() {
  tft.init();
  tft.setSwapBytes(true);
  tft.setRotation(1);
  Debug.Info("TFT init over. ");
}

void showSetupScreen() {
  tft.fillScreen(TFT_BLACK);
  File file = openSDFile("/HZK16");
  if (!file) {
    Debug.Error("read HZK16 ERROR");
  }
  displayChinese(file, 20, 20, GB.from("正在启动"), TFT_WHITE);
  file.close();
}

void setupWifi() {
  WiFi.begin(ssid, password);
  Debug.Info("Connecting to WiFi");
  int WaitCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    WaitCount++;
    if (WaitCount > WifiConnectWaitLimit) {
      Debug.Error("WiFi connect timeout");
      tft.println("WiFi connect timeout");
      while (1)
        ;
    }
  }
  Serial.print("\n");
  Debug.Debug("WiFi connected. IP address: " + WiFi.localIP().toString());
}

void setupSD() {
  if (!SD.begin()) {
    Debug.Error("SD ERROR");
    tft.drawString("SD ERROR", 5, 5);
    while (1)
      ;
  }
  Debug.Info("SD Ready");
}

void syncNTPTime() {
  Debug.Info("Start syncing time");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  int ntpRetry = 0;
  while (!getLocalTime(&timeinfo) && ntpRetry < ntpRetryLimit) {
    Debug.Warning("NTP Sync Retry");
    delay(500);
    ntpRetry++;
  }
  if (getLocalTime(&timeinfo)) {
    Debug.Info("NTP OK");
    rtc.setTimeStruct(timeinfo);
    Debug.Debug("synced time: ", true);
    Debug.Debug(rtc.getTime("%Y-%m-%d %H:%M:%S"));
  } else {
    Debug.Error("NTP sync faild");
    while (1)
      ;
  }
}

void setupWebServer() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    File file = openSDFile("/index.html");
    if (!file) {
      Debug.Warning("file not found.Request 500.");
      request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The index.html File in SD.");
      return;
    }
    file.close();
    request->send(SD, "/index.html", "text/html");
  });

  server.on("/PlanEdit/", HTTP_GET, [](AsyncWebServerRequest* request) {
    File file = openSDFile("/PlanEdit.html");
    if (!file) {
      Debug.Warning("file not found.Request 500.");
      request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The PlanEdit.html File in SD.");
      return;
    }
    file.close();
    request->send(SD, "/PlanEdit.html", "text/html");
  });

  server.on("/AJAX/planList", HTTP_GET, [](AsyncWebServerRequest* request) {
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
    }
  });

  server.on(
    "/AJAX/planList",
    HTTP_POST,
    [](AsyncWebServerRequest* request) {
      request->send(200, "application/json", "{\"code\":200,\"massage\":\"ok\"}");
      if (request->_tempObject != nullptr) {
        free(((JsonRequestBody*)request->_tempObject)->buffer);
        delete (JsonRequestBody*)request->_tempObject;
        request->_tempObject = nullptr;
      }
    },
    nullptr,
    [](AsyncWebServerRequest* request,
       uint8_t* data,  // ← 当前接收到的数据块指针
       size_t len,     // ← 当前块的长度
       size_t index,   // ← 当前块在整个body中的起始索引
       size_t total) {
      const String tag = request->header("X-Requested-With");
      if (tag != "XMLHttpRequest") {
        Debug.Warning("AJAX does not have the head");
        request->send(403, "text/plain", "403 Forbidden!");
        return;
      }
      if (index == 0) {
        auto* body = new JsonRequestBody();
        body->buffer = (uint8_t*)malloc(total + 1);  // +1 用于结尾\0
        body->totalSize = total;
        body->receivedSize = 0;
        request->_tempObject = body;
      }

      // 获取当前请求关联的缓冲区
      auto* body = (JsonRequestBody*)request->_tempObject;
      if (body && body->buffer) {
        // 拼接当前数据块
        memcpy(body->buffer + index, data, len);
        body->receivedSize += len;
      }

      // 所有数据接收完毕，开始解析
      if (index + len == total) {
        if (body && body->buffer) {
          body->buffer[total] = '\0';  // 确保字符串结束

          if (!SD.exists("/plans.json")) {
            createJson();
          }

          DynamicJsonDocument doc(512);

          DeserializationError error = deserializeJson(doc, body->buffer, body->totalSize + 1);

          if (error) {
            Debug.Warning(String("解析失败：") + String(error.c_str()));
            return;
          }

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

          fileDoc["planList"].to<JsonArray>().add(doc);

          jsonFile = SD.open("/plans.json", FILE_WRITE);
          if (!jsonFile) {
            Debug.Warning("file not open.Request 500.");
            request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The plans.json File in SD.");
            return;
          }

          serializeJsonPretty(fileDoc, jsonFile);

          jsonFile.close();
        }
      }
    });

  server.on("/AJAX/planList", HTTP_DELETE, [](AsyncWebServerRequest* request){
    const String tag = request->header("X-Requested-With");
    if (tag != "XMLHttpRequest") {
      Debug.Warning("AJAX does not have the head");
      request->send(403, "text/plain", "403 Forbidden!");
      return;
    }
    if (request->hasParam("uuid")) {
      const AsyncWebParameter* p = request->getParam("uuid");
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
      auto array = fileDoc["planList"].to<JsonArray>();

      int targetIndex = -1;

      for (int i = 0; i < array.size(); i++) {
        JsonVariant element = array[i];
        if (element.is<JsonObject>() && element.containsKey("uuid")) {
          String currentUuid = element["uuid"].as<String>();
          if (currentUuid == p->value()) {
            targetIndex = i;
            break; 
          }
        }
      }

      if(targetIndex != -1){
        array.remove(targetIndex);
        jsonFile = SD.open("/plans.json", FILE_WRITE);
          if (!jsonFile) {
            Debug.Warning("file not open.Request 500.");
            request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The plans.json File in SD.");
            return;
          }

          serializeJsonPretty(fileDoc, jsonFile);

          jsonFile.close();

          request->send(200, "json", "{ code: 200, message: '计划删除成功' }");
      }
    } else {
      request->send(400, "text/plain", "do not have the uuid");
    }
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "404 Not Found!");
  });
  server.begin();
  Debug.Info("Web Server begin");
}

void setupOTA() {
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {
        type = "filesystem";
      }

      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
      Serial.println("restart system");
      esp_restart();
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });

  ArduinoOTA.begin();
}

void keyboardLoop() {
  int upAndDown = analogRead(35);
  if (upAndDown > 2048) {

    Debug.Debug("Up Click");
  } else if (upAndDown > 32 && upAndDown < 2048) {
    Debug.Debug("Down Click");
  }
}

/**
 * 打开SD卡文件
 * @param fileName 文件名（包含路径）
 * @return 返回File对象
 */
File openSDFile(String fileName) {
  File file = SD.open(fileName, FILE_READ);
  if (!file) {
    Debug.Error("File open ERROR");
    tft.println("File open ERROR");
    return file;
  }
  Debug.Info("open SD file: " + fileName);
  return file;
}

bool createJson() {
  String templateStr = "";
  File templateFile = SD.open("/template.json", FILE_READ);
  while (templateFile.available()) {
    templateStr += templateFile.read();
  }
  templateFile.close();
  File file = SD.open("/plans.json", FILE_WRITE);
  file.write((const uint8_t*)templateStr.c_str(), templateStr.length());
  file.close();
  return true;
}