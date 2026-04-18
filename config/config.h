#include <Arduino.h>

// alarming
#define BUZZER_PIN 22

// wifi
const char *ssid = "vineky";
const char *password = "springhappyo621";
const int WifiConnectWaitLimit = 10; // times of 500ms delay

// time/rtc
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;
const char *ntpServer = "ntp.aliyun.com";
const int ntpRetryLimit = 3;
unsigned long lastNtpSync = 0;
const unsigned long NTP_SYNC_INTERVAL = 3600000; // 1 hour

// buttons
int menuPageIndex = 1;
bool preSortOK = false;

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