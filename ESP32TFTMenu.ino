#include <TFT_eSPI.h>
#include <SD.h>
#include <UTF8ToGB2312.h>
#include <WiFi.h>
#include <Debuger.h>
#include <ESP32Time.h>
#include <ESPAsyncWebServer.h>
#include <TFTMenu.h>
const char* ssid = "vineky";
const char* password = "springhappyo621";
const int WifiConnectWaitLimit = 10; // times of 500ms delay

const long gmtOffset_sec = 8*3600;
const int daylightOffset_sec = 0;
const char* ntpServer = "ntp.aliyun.com";
const int ntpRetryLimit = 3;
unsigned long lastNtpSync = 0;
const unsigned long NTP_SYNC_INTERVAL = 3600000; // 1 hour

TFT_eSPI tft;
ESP32Time rtc;
AsyncWebServer server(80);

enum SystemState{
  Initialization,
  Error,
  Screensave,
  Normal,
  Menu
};

void displayChinese(File& fontFile, int x, int y, String chStr, uint16_t color, bool noneBg = true, uint16_t bgcolor = TFT_BLACK);
void setupTFT();
void showSetupScreen();
void setupWifi();
void setupSD();
void syncNTPTime();
void setupWebServer();
File openSDFile(String fileName);
void setup() {
  Serial.begin(115200);
  delay(1000);
  setupTFT();
  setupSD();
  showSetupScreen();
  setupWifi();
  syncNTPTime();
  setupWebServer();
}

void loop() {
  if (millis() - lastNtpSync > NTP_SYNC_INTERVAL) {
    syncNTPTime(); 
    lastNtpSync = millis();
  }
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
      uint8_t dotHigh = dotMatrix[row * 2];        // 该行高8位像素数据
      uint8_t dotLow = dotMatrix[row * 2 + 1];     // 该行低8位像素数据

      for (int col = 0; col < 16; col++) {

        int charOffsetX = (i / 2) * 16; // 字符横向偏移
        int charOffsetY = 0;            // 字符纵向偏移
        
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

void setupTFT()
{
  tft.init();
  tft.setSwapBytes(true);
  tft.setRotation(1); 
  Debug.Info("TFT init over. ");
}

void showSetupScreen() {
  tft.fillScreen(TFT_BLACK);
  File file = openSDFile("/HZK16");
  if(!file){
    Debug.Error("read HZK16 ERROR");
  }
  displayChinese(file,20,20,GB.from("正在启动"),TFT_WHITE);
  file.close();
}

void setupWifi() {
  WiFi.begin(ssid, password);
  Debug.Info("Connecting to WiFi");
  int WaitCount = 0;
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    WaitCount++;
    if(WaitCount > WifiConnectWaitLimit){
      Debug.Error("WiFi connect timeout");
      tft.println("WiFi connect timeout");
      while(1);
    }
  }
  Serial.print("\n");
  Debug.Debug("WiFi connected. IP address: " + WiFi.localIP().toString());
}

void setupSD(){
  if (!SD.begin()) {
    Debug.Error("SD ERROR");
    tft.drawString("SD ERROR",5,5);
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
    Debug.Debug("synced time: ",true);
    Debug.Debug(rtc.getTime("%Y-%m-%d %H:%M:%S"));
  } else {
    Debug.Error("NTP sync faild");
    while (1) ;
  }
}

void setupWebServer() {
  server.on("/",HTTP_GET,[](AsyncWebServerRequest *request){
    File file = openSDFile("/index.html");
    if(!file){
      Debug.Warning("file not found.Request 404.");
      request->send(404, "text/plain", "404 Not Found!");
      return ;
    }
    file.close();
    request->send(SD, "/index.html", "text/html");
    
  });
  server.on("/PlanEdit/",HTTP_GET,[](AsyncWebServerRequest *request){
    File file = openSDFile("/PlanEdit.html");
    if(!file){
      Debug.Warning("file not found.Request 404.");
      request->send(404, "text/plain", "404 Not Found!");
      return ;
    }
    file.close();
    request->send(SD, "/PlanEdit.html", "text/html");
  });
  server.on("/AJAX/planList",HTTP_GET,[](AsyncWebServerRequest *request){
    Debug.Info("Received a planList AJAX");
    const String tag = request->header("X-Requested-With");
    if(tag.isEmpty()){
      Debug.Warning("AJAX does not have the head");
      request->send(403,"text/plain","403 Forbidden!");
    }else{
      if(tag != "XMLHttpRequest"){
        Debug.Warning("AJAX does not have the right head");
        request->send(403,"text/plain","403 Forbidden!");
      }else{
        File file = openSDFile("/plans.json");
        if(!file){
          Debug.Warning("file not found.Request 404.");
          request->send(404, "text/plain", "404 Not Found!");
          return ;
        }
        file.close();
        Debug.Info("AJAX is right.Send the request.");
        request->send(SD, "/plans.json", "application/json");
      }
    }
  });
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "404 Not Found!");
  });
  server.begin();
  Debug.Info("Web Server begin");
}

/**
 * 打开SD卡文件
 * @param fileName 文件名（包含路径）
 * @return 返回File对象
 */
File openSDFile(String fileName){
  File file = SD.open(fileName, FILE_READ);
  if (!file) {
    Debug.Error("File open ERROR");
    tft.println("File open ERROR");
    return file;
  }
  Debug.Info("open SD file: " + fileName);
  return file;
}