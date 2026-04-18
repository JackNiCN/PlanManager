#ifndef CONFIG_H
#define CONFIG_H

#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_USE_FLOAT 0
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <TFT_eSPI.h>
#include <SD.h>
#include <UTF8ToGB2312.h>
#include <Debuger.h>
#include <ESP32Time.h>
#include <ArduinoOTA.h>
#include "tools/text/text.h"
#include <CircularBuffer.hpp>

#include "../tools/alarm/alarm.h"
#include "../tools/sd/sd.h"
#include "../tools/text/text.h"

class Config
{
public:
  //alarming
  Alarm alarm;

  //wifi
  const char *ssid = "vineky";
  const char *password = "springhappyo621";
  const int WifiConnectWaitLimit = 10; // times of 500ms delay

  //time/rtc
  const long gmtOffset_sec = 8 * 3600;
  const int daylightOffset_sec = 0;
  const char *ntpServer = "ntp.aliyun.com";
  const int ntpRetryLimit = 3;
  unsigned long lastNtpSync = 0;
  const unsigned long NTP_SYNC_INTERVAL = 3600000; // 1 hour

  //buttons
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

  enum ButtonName
  {
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
  File uploadFile;
  TFTMenu menu(&tft, &spr, 50);

  enum SystemState
  {
    Initialization,
    Error,
    Screensave,
    Normal,
    Menu,
    MoreInfo
  };

  struct JsonRequestBody
  {
    uint8_t *buffer = nullptr;
    size_t totalSize = 0;
    size_t receivedSize = 0;
  };
}

Config::Config()
{
  //alarming default config
  alarm.is_alarming = false;    // 报警总开关
  alarm.alarm_start_time = 0;   // 报警启动时间戳
  alarm.ALARM_DURATION = 30000; // 报警持续时间（30秒）
  alarm.UZZER_ON_TIME = 300;    // 蜂鸣器响的时长（毫秒）
  alarm.BUZZER_OFF_TIME = 700;  // 蜂鸣器停的时长（毫秒）

}

#endif