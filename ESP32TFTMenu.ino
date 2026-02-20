#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_USE_FLOAT 0
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
#include "TextWrite.h"
#include <CircularBuffer.hpp>
const char *ssid = "vineky";
const char *password = "springhappyo621";
const int WifiConnectWaitLimit = 10;  // times of 500ms delay

const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;
const char *ntpServer = "ntp.aliyun.com";
const int ntpRetryLimit = 3;
unsigned long lastNtpSync = 0;
const unsigned long NTP_SYNC_INTERVAL = 3600000;  // 1 hour

int upButtonLastClickedTime = 0;
int downButtonLastClickedTime = 0;
int leftButtonLastClickedTime = 0;
int rightButtonLastClickedTime = 0;
int middleButtonLastClickedTime = 0;
int extButtonLastClickedTime = 0;
bool upButtonLastState = 0;
bool downButtonLastState = 0;
bool leftButtonLastState = 0;
bool rightButtonLastState = 0;
bool middleButtonLastState = 0;
bool extButtonLastState = 0;

enum ButtonName {
  UP_BUTTON,
  DOWN_BUTTON,
  LEFT_BUTTON,
  RIGHT_BUTTON,
  MIDDLE_BUTTON,
  EXT_BUTTON
};

CircularBuffer<ButtonName, 10> buttonQueue;

TFT_eSPI tft;
ESP32Time rtc;
AsyncWebServer server(80);
TFTMenu menu(&tft, 50);

enum SystemState {
  Initialization,
  Error,
  Screensave,
  Normal,
  Menu
};

struct JsonRequestBody {
  uint8_t *buffer = nullptr;
  size_t totalSize = 0;
  size_t receivedSize = 0;
};

struct PlanItem {
  String name;
  time_t startTime;
  time_t endTime;
  String info;
  String id;
};


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

SystemState sysState;
bool isFirstRenderMainScreen = 1;
unsigned long lastRSSIDraw = 0;
unsigned long lastPlanCheck = 0;
void renderTFT();
void doRenderMain();
int getSignalLevel(int rssi);
void drawWiFiIcon(int x, int y, int level);
PlanItem FindPlanbyTime(time_t);
time_t stringToTime(const String &dateStr, const String &timeStr);
void setup() {
  Serial.begin(115200);
  delay(1000);
  Debug.Info("system start");
  sysState = SystemState::Initialization;
  setupTFT();
  setupSD();
  showSetupScreen();
  setupWifi();
  setupOTA();
  syncNTPTime();
  setupWebServer();


  menu.addItem("hello");
  menu.showMenu(1, 1, 159, 127, 1, TFT_WHITE, TFT_BLACK);
  delay(10000);
  sysState = SystemState::Normal;
}

void loop() {
  if (millis() - lastNtpSync > NTP_SYNC_INTERVAL) {
    syncNTPTime();
    lastNtpSync = millis();
  }
  ArduinoOTA.handle();
  keyboardLoop();
  renderTFT();
  delay(50);
}

void setupTFT() {
  tft.init();
  tft.setSwapBytes(true);
  tft.setRotation(1);
  Text.setTFTClass(&tft);
  Debug.Info("TFT init over. ");
}

void showSetupScreen() {
  tft.fillScreen(TFT_BLACK);
  File file = openSDFile("/HZK16");
  if (!file) {
    Debug.Error("read HZK16 ERROR");
  }
  Text.WriteText(file, "正在启动", 20, 20, TFT_WHITE);
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
      sysState = SystemState::Error;
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
    sysState = SystemState::Error;
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
    sysState = SystemState::Error;
    while (1)
      ;
  }
}

void setupWebServer() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    File file = openSDFile("/index.html");
    if (!file) {
      Debug.Warning("file not found.Request 500.");
      request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The index.html File in SD.");
      return;
    }
    file.close();
    request->send(SD, "/index.html", "text/html");
  });

  server.on("/PlanEdit/", HTTP_GET, [](AsyncWebServerRequest *request) {
    File file = openSDFile("/PlanEdit.html");
    if (!file) {
      Debug.Warning("file not found.Request 500.");
      request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The PlanEdit.html File in SD.");
      return;
    }
    file.close();
    request->send(SD, "/PlanEdit.html", "text/html");
  });

  server.on("/AJAX/planList", HTTP_GET, [](AsyncWebServerRequest *request) {
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
    [](AsyncWebServerRequest *request) {
      request->send(200, "application/json", "{\"code\":200,\"massage\":\"ok\"}");
      if (request->_tempObject != nullptr) {
        free(((JsonRequestBody *)request->_tempObject)->buffer);
        delete (JsonRequestBody *)request->_tempObject;
        request->_tempObject = nullptr;
      }
    },
    nullptr,
    [](AsyncWebServerRequest *request,
       uint8_t *data,  // ← 当前接收到的数据块指针
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
        auto *body = new JsonRequestBody();
        body->buffer = (uint8_t *)malloc(total + 1);  // +1 用于结尾\0
        body->totalSize = total;
        body->receivedSize = 0;
        request->_tempObject = body;
      }

      // 获取当前请求关联的缓冲区
      auto *body = (JsonRequestBody *)request->_tempObject;
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

          fileDoc["planList"].as<JsonArray>().add(doc);

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

  server.on("/AJAX/planList", HTTP_DELETE, [](AsyncWebServerRequest *request) {
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
    auto array = fileDoc["planList"].to<JsonArray>();
    int targetIndex = -1;
    for (int i = 0; i < array.size(); i++) {
      JsonVariant element = array[i];
      if (element.is<JsonObject>() && element.containsKey("uuid")) {
        String currentUuid = element["uuid"].as<String>();
        if (currentUuid == p) {
          targetIndex = i;
          break;
        }
      }
    }
    if (targetIndex != -1) {
      array.remove(targetIndex);
      jsonFile = SD.open("/plans.json", FILE_WRITE);
      if (!jsonFile) {
        Debug.Warning("file not open.Request 500.");
        request->send(500, "text/plain", "500 Server ERROR!Connot Find and Open The plans.json File in SD.");
        return;
      }
      serializeJsonPretty(fileDoc, jsonFile);
      jsonFile.close();
    }
    request->send(200, "json", "{ code: 200, message: '计划删除成功' }");
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.printf("404错误 - 请求方法：%s，请求路径：%s\n", 
    request->methodToString(), request->url().c_str());
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
    if (upButtonLastState == 0) {
      Debug.Debug("Up Click");
      buttonQueue.push(ButtonName::UP_BUTTON);
    }
  } else if (upAndDown > 32 && upAndDown < 2048) {
    if (downButtonLastState == 0) {
      Debug.Debug("Down Click");
      buttonQueue.push(ButtonName::DOWN_BUTTON);
    }
  } else {
    upButtonLastState = 0;
    downButtonLastState = 0;
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
  file.write((const uint8_t *)templateStr.c_str(), templateStr.length());
  file.close();
  return true;
}

void renderTFT() {
  if (sysState == SystemState::Normal) {
    doRenderMain();
  } else {
    isFirstRenderMainScreen = false;
  }
}

void doRenderMain() {
  if (isFirstRenderMainScreen) {
    tft.fillScreen(TFT_BLACK);
    Text.setTFTClass(&tft);
    File file = openSDFile("/HZK16");
    if (file) {
      Text.displayChinese(file, 2, 24, GB.from("当前计划："), TFT_WHITE, false, TFT_BLACK);
      file.close();
      tft.drawLine(0, 22, 160, 22, TFT_WHITE);
    } else {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawString("Connot open HZK16", 5, 24);
    }
    isFirstRenderMainScreen = false;
    file.close();
  }
  time_t timestamp = rtc.getEpoch();
  struct tm *timeinfo = localtime(&timestamp);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(timeinfo->tm_hour) + ":" + String(timeinfo->tm_min), 1, 1);

  if (millis() - lastPlanCheck > 30000) {
    auto plan = FindPlanbyTime(timestamp);
    if (plan.name == "fail") {
      Debug.Warning("Get current plan faild");
    } else if (plan.name == "unknow") {
      File file = openSDFile("/HZK16");
      if (file) {
        Text.WriteText(file, "没有计划", 86, 24, TFT_WHITE);
      } else {
        Debug.Warning("File open Error");
      }
      file.close();
    }else{
      File file = openSDFile("/HZK16");
      if (file) {
        Text.WriteText(file, plan.name, 0, 40, TFT_WHITE);
      } else {
        Debug.Warning("File open Error");
      }
      file.close();
    }
    lastPlanCheck = millis();
  }

  if (millis() - lastRSSIDraw > 10000) {
    drawWiFiIcon(15, 160 - 12, getSignalLevel(WiFi.RSSI()));
    Debug.Debug(String(WiFi.RSSI()));
    lastRSSIDraw = millis();
  }
}

int getSignalLevel(int rssi) {
  if (rssi >= -50)
    return 4;
  else if (rssi >= -60)
    return 3;
  else if (rssi >= -70)
    return 2;
  else if (rssi >= -80)
    return 1;
  else
    return 0;
}

void drawWiFiIcon(int x, int y, int level) {
  tft.fillRect(y - 8, x - 10, 16, 16, TFT_BLACK);
  tft.fillCircle(y, x, 2, level > 0 ? TFT_BLUE : 0x39C4);
  if (level >= 1)
    tft.fillRect(y - 1, x - 2, 3, 2, TFT_BLUE);
  else if (level > 0)
    tft.drawRect(y - 1, x - 2, 3, 2, 0x39C4);
  if (level >= 2)
    tft.fillRect(y - 3, x - 5, 6, 2, TFT_BLUE);
  else if (level > 0)
    tft.drawRect(y - 3, x - 5, 6, 2, 0x39C4);
  if (level >= 3)
    tft.fillRect(y - 4, x - 8, 9, 2, TFT_BLUE);
  else if (level > 0)
    tft.drawRect(y - 4, x - 8, 9, 2, 0x39C4);
  if (level >= 4)
    tft.fillRect(y - 6, x - 11, 12, 2, TFT_BLUE);
  else if (level > 0)
    tft.drawRect(y - 6, x - 11, 12, 2, 0x39C4);
}

PlanItem FindPlanbyTime(time_t time) {
  File jsonFile = SD.open("/plans.json", FILE_READ);
  if (!jsonFile) {
    Debug.Error("file not open.Connot find plan.");
    return { "fail", 0, 0, "fail", "fail" };
  }

  size_t fileSize = jsonFile.size();
  DynamicJsonDocument doc(fileSize * 4);

  DeserializationError error1 = deserializeJson(doc, jsonFile);
  jsonFile.close();
  if (error1) {
    Debug.Error("解析JSON文件错误");
    return { "fail", 0, 0, "fail", "fail" };
  }
  serializeJsonPretty(doc["planList"], Serial);
  Debug.Debug(String(doc["planList"].size()));
  for (int i = 0; i < doc["planList"].size(); i++) {
    String dateS = doc["planList"][i]["date"].as<String>();
    String bS = doc["planList"][i]["beginTime"].as<String>();
    String eS = doc["planList"][i]["endTime"].as<String>();
    Serial.printf("DEBUG plan[%d] date='%s' begin='%s' end='%s'\n", i, dateS.c_str(), bS.c_str(), eS.c_str());

    time_t startTime = stringToTime(dateS, bS);
    time_t endTime = stringToTime(dateS, eS);
    Serial.printf("PlanCheck:%ld-%ld curr:%ld\n", (long)startTime, (long)endTime, (long)time);
    if (time >= startTime && time <= endTime) {
      return { doc["planList"][i]["name"].as<String>(), startTime, endTime, doc["planList"][i]["description"].as<String>(), doc["planList"][i]["id"].as<String>() };
    }
  }

  return { "unknow", 0, 0, "unknow", "unknow" };
}

time_t stringToTime(const String &dateStr, const String &timeStr) {
  // 1. 解析日期字符串 YYYY-MM-DD
  int year = dateStr.substring(0, 4).toInt();
  int month = dateStr.substring(5, 7).toInt();
  int day = dateStr.substring(8, 10).toInt();

  // 2. 解析时间字符串 HH:MM
  int hour = timeStr.substring(0, 2).toInt();
  int minute = timeStr.substring(3, 5).toInt();
  int second = 0;  // 未提供秒数，默认设为0

  // 3. 校验输入格式的合法性（基础校验）
  if (dateStr.length() != 10 || timeStr.length() != 5 || dateStr.charAt(4) != '-' || dateStr.charAt(7) != '-' || timeStr.charAt(2) != ':' || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    Serial.println("错误：日期或时间格式非法！");
    return 0;  // 返回0表示解析失败
  }

  // 4. 填充tm结构体（tm_mon从0开始，所以月份要-1；tm_year是从1900年开始的年数）
  struct tm timeInfo = { 0 };
  timeInfo.tm_sec = second;        // 秒
  timeInfo.tm_min = minute;        // 分
  timeInfo.tm_hour = hour;         // 时
  timeInfo.tm_mday = day;          // 日
  timeInfo.tm_mon = month - 1;     // 月（0-11）
  timeInfo.tm_year = year - 1900;  // 年（从1900开始）
  timeInfo.tm_isdst = -1;          // 自动判断夏令时

  // 5. 转换为time_t时间戳
  time_t timestamp = mktime(&timeInfo);

  // 校验转换结果
  if (timestamp == (time_t)-1) {
    Serial.println("错误：时间转换失败（非法日期，如2月30日）！");
    return 0;
  }

  return timestamp;
}