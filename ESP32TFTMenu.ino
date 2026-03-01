/*
 * Copyright (c) 2026 Jack Ni
 *
 * This file is part of PlanManager.
 *
 * Licensed under the MIT License (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#define BUZZER_PIN 22
bool is_alarming = false;          // 报警总开关
unsigned long alarm_start_time = 0; // 报警启动时间戳
const unsigned long ALARM_DURATION = 30000; // 报警持续时间（30秒）
const int BUZZER_ON_TIME = 300;     // 蜂鸣器响的时长（毫秒）
const int BUZZER_OFF_TIME = 700;  // 蜂鸣器停的时长（毫秒）

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
int menuPageIndex = 1;
bool preSortOK = false;

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
TFT_eSprite spr(&tft);
ESP32Time rtc;
AsyncWebServer server(80);
TFTMenu menu(&tft, &spr, 50);

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
  long durationMinutes;
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
bool isFirstRenderMainScreen = true;
bool MenuChanged = true;
unsigned long lastRSSIDraw = 0;
unsigned long lastPlanCheck = 0;
String lastTimeStr;
void renderTFT();
void doRenderMain();
void doRenderScreenSave();
void doRenderMenu();
int getSignalLevel(int rssi);
void drawWiFiIcon(int x, int y, int level);
PlanItem FindPlanbyTime(time_t);
time_t stringToTime(const String &dateStr, const String &timeStr);
void promptTone();
void quickSortJsonArray(JsonArray arr, int low, int high, bool (*comparator)(JsonVariant a, JsonVariant b));
void sortJsonArrayByVariantQuick(JsonArray jsonArray, bool (*comparator)(JsonVariant a, JsonVariant b));
bool planListComparator(JsonVariant a, JsonVariant b);
int insertIntoSortedJsonArray(JsonArray sortedArray, JsonVariant newElement, bool (*comparator)(JsonVariant a, JsonVariant b)) ;
void preSortPlanList();

void buzzer_alarm_task(void *parameter) {
  // 任务循环
  while (true) {
    if (is_alarming) {
      if (millis() - alarm_start_time >= ALARM_DURATION) {
        is_alarming = false;
        digitalWrite(BUZZER_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
        vTaskDelay(BUZZER_ON_TIME / portTICK_PERIOD_MS); // 响指定时长
        digitalWrite(BUZZER_PIN, HIGH);
        vTaskDelay(BUZZER_OFF_TIME / portTICK_PERIOD_MS); // 停指定时长
      }
    } else {
      // 无报警时，蜂鸣器关闭
      digitalWrite(BUZZER_PIN, HIGH);
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}


void start_alarm() {
  if (!is_alarming) { 
    is_alarming = true;
    alarm_start_time = millis();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Debug.Info("system start");
  sysState = SystemState::Initialization;
  setupTFT();
  setupSD();
  showSetupScreen();

  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  delay(1000);
  digitalWrite(22, HIGH);

  xTaskCreate(
    buzzer_alarm_task,    // 任务函数
    "BuzzerAlarm",        // 任务名称
    2048,                 // 栈大小
    NULL,                 // 传递给任务的参数
    2,                    // 任务优先级
    NULL                  // 任务句柄
  );

  setupWifi();
  //setupOTA();
  syncNTPTime();
  setupWebServer();
  preSortPlanList();
  menu.setWindowPosition(0, 0, 160, 128);
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
  delay(100);
}

void setupTFT() {
  tft.init();
  tft.setSwapBytes(true);
  tft.setRotation(1);
  spr.createSprite(160, 128);
  Text.setTFTClass(&tft);
  Text.setSprite(&spr);
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

          insertIntoSortedJsonArray(fileDoc["planList"], doc, planListComparator);

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
      upButtonLastState = 1;
      Debug.Debug("Up Click");
      buttonQueue.push(ButtonName::UP_BUTTON);
    }
  } else if (upAndDown > 32 && upAndDown < 2048) {
    if (downButtonLastState == 0) {
      downButtonLastState = 1;
      Debug.Debug("Down Click");
      buttonQueue.push(ButtonName::DOWN_BUTTON);
    }
  } else {
    upButtonLastState = 0;
    downButtonLastState = 0;
  }
  int leftAndRight = analogRead(32);
  if (leftAndRight > 2048) {
    if (leftButtonLastState == 0) {
      leftButtonLastState = 1;
      Debug.Debug("Left Click");
      buttonQueue.push(ButtonName::LEFT_BUTTON);
    }
  } else if (leftAndRight > 32 && leftAndRight < 2048) {
    if (rightButtonLastState == 0) {
      rightButtonLastState = 1;
      Debug.Debug("Right Click");
      buttonQueue.push(ButtonName::RIGHT_BUTTON);
    }
  } else {
    leftButtonLastState = 0;
    rightButtonLastState = 0;
  }

  int midAndExt = analogRead(34);
  if (midAndExt > 2048) {
    if (middleButtonLastState == 0) {
      middleButtonLastState = 1;
      Debug.Debug("Middle Click");
      buttonQueue.push(ButtonName::MIDDLE_BUTTON);
    }
  } else if (midAndExt > 32 && midAndExt < 2048) {
    if (extButtonLastState == 0) {
      extButtonLastState = 1;
      Debug.Debug("Ext Click");
      buttonQueue.push(ButtonName::EXT_BUTTON);
    }
  } else {
    middleButtonLastState = 0;
    extButtonLastState = 0;
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
  if (sysState == SystemState::Screensave) {
    if (!buttonQueue.isEmpty()) {
      buttonQueue.clear();
      sysState = SystemState::Normal;
      isFirstRenderMainScreen = true;
      lastPlanCheck = 0;
      lastRSSIDraw = 0;
      return;
    }
  }
  if (sysState == SystemState::Menu) {
    doRenderMenu();
  }
}

void doRenderMain() {
  if (!buttonQueue.isEmpty()) {
    is_alarming = false;
    if (buttonQueue.pop() == ButtonName::EXT_BUTTON) {
      sysState = SystemState::Menu;
      buttonQueue.clear();
      MenuChanged = true;
    }
  }

  Text.setTFTClass(&tft);
  Text.setSprite(&spr);
  if (isFirstRenderMainScreen) {
    spr.fillSprite(TFT_BLACK);
    File file = openSDFile("/HZK16");
    if (file) {
      Text.displayChinese(file, 2, 24, GB.get("当前计划："), TFT_WHITE, false, TFT_BLACK);
      file.close();
      spr.drawLine(0, 22, 160, 22, TFT_WHITE);
    } else {
      spr.setTextColor(TFT_RED, TFT_BLACK);
      spr.drawString("Connot open HZK16", 5, 24);
    }
    isFirstRenderMainScreen = false;
    file.close();
  }
  String time;
  time_t timestamp = rtc.getEpoch();
  struct tm *tmpPtr = localtime(&timestamp);
  struct tm timeinfo;
  if (tmpPtr)
        memcpy(&timeinfo, tmpPtr, sizeof(struct tm));
      else
        memset(&timeinfo, 0, sizeof(struct tm));
  time = (timeinfo.tm_hour < 10 ? "0" + String(timeinfo.tm_hour) : String(timeinfo.tm_hour)) + ":" + (timeinfo.tm_min < 10 ? "0" + String(timeinfo.tm_min) : String(timeinfo.tm_min));
  if (time != lastTimeStr) {
    lastTimeStr = time;
    spr.setTextSize(2);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawString(time, 1, 1);

    PlanItem plan = FindPlanbyTime(timestamp);
    static PlanItem lastFindPlan;
    lastFindPlan = plan;
    spr.fillRect(0, 40, 160, 80, TFT_BLACK);
    spr.fillRect(86, 24, 80, 16, TFT_BLACK);
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
    } else {
      {
        File file = openSDFile("/HZK16");
        if (file) {
          Text.WriteText(file, plan.name, 0, 40, TFT_WHITE);
        } else {
          Debug.Warning("File open Error");
        }
        file.close();
      }
      struct tm STbuf;
      struct tm ETbuf;


      tmpPtr = localtime(&plan.startTime);
      if (tmpPtr)
        memcpy(&STbuf, tmpPtr, sizeof(struct tm));
      else
        memset(&STbuf, 0, sizeof(struct tm));

      tmpPtr = localtime(&plan.endTime);
      if (tmpPtr)
        memcpy(&ETbuf, tmpPtr, sizeof(struct tm));
      else
        memset(&ETbuf, 0, sizeof(struct tm));

      spr.setTextSize(1);
      String timeStr = (STbuf.tm_hour < 10 ? "0" + String(STbuf.tm_hour) : String(STbuf.tm_hour)) + ":" + (STbuf.tm_min < 10 ? "0" + String(STbuf.tm_min) : String(STbuf.tm_min));
      timeStr += " - ";
      timeStr += (ETbuf.tm_hour < 10 ? "0" + String(ETbuf.tm_hour) : String(ETbuf.tm_hour)) + ":" + (ETbuf.tm_min < 10 ? "0" + String(ETbuf.tm_min) : String(ETbuf.tm_min));
      spr.drawString(timeStr, 1, 60);
      spr.fillRect(1, 76, 16, 159, TFT_BLACK);
      File file = openSDFile("/HZK16");
      if (file) {
        Text.WriteText(file, "时长:" + String(plan.durationMinutes), 1, 76, TFT_WHITE);
      } else {
        Debug.Warning("File open Error");
      }
      file.close();
      if(STbuf.tm_hour == timeinfo.tm_hour && STbuf.tm_min == timeinfo.tm_min){
        start_alarm();
      }
    }
  }
  if (millis() - lastRSSIDraw > 10000 || lastRSSIDraw == 0) {
    drawWiFiIcon(15, 160 - 12, getSignalLevel(WiFi.RSSI()));
    Debug.Debug(String(WiFi.RSSI()));
    lastRSSIDraw = millis();
  }
  spr.pushSprite(0, 0);
}

void doRenderScreenSave() {
}

void doRenderMenu() {

  if (!buttonQueue.isEmpty()) {
    ButtonName currentButton = buttonQueue.pop();


    if (currentButton == ButtonName::EXT_BUTTON) {
      sysState = SystemState::Normal;
      isFirstRenderMainScreen = true;
      lastPlanCheck = 0;
      lastRSSIDraw = 0;
      lastTimeStr = "";
      buttonQueue.clear();
      promptTone();
    }
    if (currentButton == ButtonName::UP_BUTTON) {
      menu.itemUp();
      promptTone();
      MenuChanged = true;
    }
    if (currentButton == ButtonName::DOWN_BUTTON) {
      menu.itemDown();
      promptTone();
      MenuChanged = true;
    }
    if (currentButton == ButtonName::LEFT_BUTTON) {
      if(menuPageIndex > 1){
        menuPageIndex-- ;
      }
      promptTone();
      MenuChanged = true;
    }
    if (currentButton == ButtonName::RIGHT_BUTTON) {
      menuPageIndex++ ;
      promptTone();
      MenuChanged = true;
    }
  }
  if (MenuChanged) {
    menu.clearItemList();
    File jsonFile = SD.open("/plans.json", FILE_READ);
    if (!jsonFile) {
      Debug.Error("file not open.Connot find plan.");
    }

    size_t fileSize = jsonFile.size();
    DynamicJsonDocument doc(fileSize * 4);

    DeserializationError error1 = deserializeJson(doc, jsonFile);
    jsonFile.close();
    if (error1) {
      Debug.Error("解析JSON文件错误");
      menu.addItem("解析JSON文件错误");
    }
    serializeJsonPretty(doc["planList"], Serial);
    Debug.Debug(String(doc["planList"].size()));
    for (int i = 0; i < doc["planList"].size(); i++) {
      menu.addItem(doc["planList"][i]["name"]);
    }

    menu.showMenu(menuPageIndex, TFT_BLUE, TFT_BLACK);
    MenuChanged = false;
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
  spr.fillRect(y - 8, x - 10, 16, 16, TFT_BLACK);
  spr.fillCircle(y, x, 2, level > 0 ? TFT_BLUE : 0x39C4);
  if (level >= 1)
    spr.fillRect(y - 1, x - 2, 3, 2, TFT_BLUE);
  else if (level > 0)
    spr.drawRect(y - 1, x - 2, 3, 2, 0x39C4);
  if (level >= 2)
    spr.fillRect(y - 3, x - 5, 6, 2, TFT_BLUE);
  else if (level > 0)
    spr.drawRect(y - 3, x - 5, 6, 2, 0x39C4);
  if (level >= 3)
    spr.fillRect(y - 4, x - 8, 9, 2, TFT_BLUE);
  else if (level > 0)
    spr.drawRect(y - 4, x - 8, 9, 2, 0x39C4);
  if (level >= 4)
    spr.fillRect(y - 6, x - 11, 12, 2, TFT_BLUE);
  else if (level > 0)
    spr.drawRect(y - 6, x - 11, 12, 2, 0x39C4);
}

PlanItem FindPlanbyTime(time_t time) {
  File jsonFile = SD.open("/plans.json", FILE_READ);
  if (!jsonFile) {
    Debug.Error("file not open.Connot find plan.");
    return { "fail", 0, 0, 0, "fail", "fail" };
  }

  size_t fileSize = jsonFile.size();
  DynamicJsonDocument doc(fileSize * 4);

  DeserializationError error1 = deserializeJson(doc, jsonFile);
  jsonFile.close();
  if (error1) {
    Debug.Error("解析JSON文件错误");
    return { "fail", 0, 0, 0, "fail", "fail" };
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
    long durationMinutes = doc["planList"][i]["durationMinutes"].as<long>();
    Serial.printf("PlanCheck:%ld-%ld curr:%ld\n", (long)startTime, (long)endTime, (long)time);
    if (time >= startTime && time <= endTime) {
      return { doc["planList"][i]["name"].as<String>(), startTime, durationMinutes, endTime, doc["planList"][i]["description"].as<String>(), doc["planList"][i]["id"].as<String>() };
    }
  }

  return { "unknow", 0, 0, 0, "unknow", "unknow" };
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

void promptTone(){
  digitalWrite(BUZZER_PIN, LOW);
  delay(150);
  digitalWrite(BUZZER_PIN, HIGH);
}

void quickSortJsonArray(JsonArray arr, int low, int high, bool (*comparator)(JsonVariant a, JsonVariant b)) {
  if (low >= high) return;

  // 选基准值 ——必须复制值，不能只是拿到引用
  // JsonVariant pivot = arr[high]; // 原来这只是引用, 后续交换会改变它
  DynamicJsonDocument pivotDoc(256);
  JsonVariant pivot = pivotDoc.to<JsonVariant>();
  pivot.set(arr[high]);

  int i = low - 1;

  // 分区
  for (int j = low; j < high; j++) {
    if (comparator(arr[j], pivot)) {
      i++;
      // 交换arr[i]和arr[j]，使用临时 JsonVariant 深拷贝
      DynamicJsonDocument tmpDoc(256);
      JsonVariant temp = tmpDoc.to<JsonVariant>();
      temp.set(arr[i]);     // 保存原始 arr[i]
      arr[i].set(arr[j]);   // arr[i] = arr[j]
      arr[j].set(temp);     // arr[j] = old arr[i]
    }
  }

  // 交换基准值到正确位置，同样使用深拷贝
  i++;
  {
    DynamicJsonDocument tmpDoc(256);
    JsonVariant temp = tmpDoc.to<JsonVariant>();
    temp.set(arr[i]);
    arr[i].set(arr[high]);
    arr[high].set(temp);
  }

  // 递归排序左右分区
  quickSortJsonArray(arr, low, i - 1, comparator);
  quickSortJsonArray(arr, i + 1, high, comparator);
}

// 对外封装的快速排序函数
void sortJsonArrayByVariantQuick(JsonArray jsonArray, bool (*comparator)(JsonVariant a, JsonVariant b)) {
  if (jsonArray.size() <= 1) return;
  quickSortJsonArray(jsonArray, 0, jsonArray.size() - 1, comparator);
}

bool planListComparator(JsonVariant a, JsonVariant b)
{
  String AdateS = a["date"].as<String>();
  String AbeginS = a["beginTime"].as<String>();
  time_t AstartTime = stringToTime(AdateS, AbeginS);
  String BdateS = b["date"].as<String>();
  String BbeginS = b["beginTime"].as<String>();
  time_t BstartTime = stringToTime(BdateS, BbeginS);
  return AstartTime < BstartTime;
}
/**
 * @brief 向已有序的JSON数组中插入新元素（保持数组有序）
 * @param sortedArray 已排序的JsonArray（升序/降序由comparator决定）
 * @param newElement  要插入的新元素（JsonVariant）
 * @param comparator  比较函数（a应排在b前则返回true，如升序：a<int>() < b<int>()）
 * @return int        新元素插入的索引位置
 */
int insertIntoSortedJsonArray(JsonArray sortedArray, JsonVariant newElement, bool (*comparator)(JsonVariant a, JsonVariant b)) {
  if(!preSortOK){
    return -1;
  }
  size_t n = sortedArray.size();
  
  int insertIndex = 0;
  for (; insertIndex < n; insertIndex++) {
    if (!comparator(sortedArray[insertIndex], newElement)) {
      break;
    }
  }

  sortedArray.add(JsonVariant());

  for (int i = n; i > insertIndex; i--) {
    sortedArray[i].set(sortedArray[i-1]);
  }

  sortedArray[insertIndex].set(newElement);

  return insertIndex;
}

void preSortPlanList()
{
  File jsonFile = SD.open("/plans.json", FILE_READ);
  if (!jsonFile)
  {
    Debug.Warning("file not open.All Sort Will Not Be used.");
    return;
  }

  size_t fileSize = jsonFile.size();

  DynamicJsonDocument fileDoc(fileSize * 2);

  DeserializationError error1 = deserializeJson(fileDoc, jsonFile);
  jsonFile.close();
  if (error1)
  {
    Debug.Warning("deserializeJson file faild.All Sort Will Not Be used.");
    return;
  }
  sortJsonArrayByVariantQuick(fileDoc["planList"], planListComparator);
  jsonFile = SD.open("/plans.json", FILE_WRITE);
  if (!jsonFile)
  {
    Debug.Warning("file not open.");
    return;
  }
  serializeJsonPretty(fileDoc, jsonFile);
  jsonFile.close();
  preSortOK = true;
}
