#include "sd.h"

void Sd::setupSD(TFT_eSPI tft) {
  if (!SD.begin()) {
    Debug.Error("SD ERROR");
    tft.drawString("SD ERROR", 5, 5);
    sysState = SystemState::Error;
    while (1)
      ;
  }
  Debug.Info("SD Ready");
}

/**
 * 打开SD卡文件
 * @param fileName 文件名（包含路径）
 * @return 返回File对象
 */
File Sd::openSDFile(String fileName, TFT_eSPI tft) {
  File file = SD.open(fileName, FILE_READ);
  if (!file) {
    Debug.Error("File open ERROR");
    tft.println("File open ERROR");
    return file;
  }
  Debug.Info("open SD file: " + fileName);
  return file;
}
