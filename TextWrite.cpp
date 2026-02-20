#include "TextWrite.h"

TextWrite Text;

TextWrite::TextWrite() {
  this->tft = nullptr;
}

void TextWrite::setTFTClass(TFT_eSPI *_tft) {
  this->tft = _tft;
}

void TextWrite::WriteText(File &fontFile, String text, int x, int y, uint16_t color, bool noneBg, uint16_t bgcolor) {
  SplitChinese::TextUnit units[20];
  int count = 0;
  spliter.splitChineseAndASCII(text, units, count);
  int charX = x, charY = y;
  for (int i = 0; i < count; i++) {
    if (units[i].type == SplitChinese::TYPE_CHINESE) {
      Serial.println(String(i) + ":" + units[i].content + "\n类型" + String(units[i].type) + "\n在(x,y)" + String(charX) + ',' + String(charY));
      displayChinese(fontFile, charX, charY, GB.from(units[i].content), color, noneBg, bgcolor);
      charX += units[i].content.length() / 3 * 16;
    } else {
      Serial.println(String(i) + ":" + units[i].content + "\n类型" + String(units[i].type) + "\n在(x,y)" + String(charX) + ',' + String(charY));
      tft->setTextColor(color, bgcolor, !noneBg);
      tft->setTextSize(2);
      tft->drawString(units[i].content, charX, charY);
      charX += tft->textWidth(units[i].content);
    }
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
void TextWrite::displayChinese(File &fontFile, int x, int y, String chStr, uint16_t color, bool noneBg, uint16_t bgcolor) {
  int strLen = chStr.length();

  for (int i = 0; i < strLen; i += 2) {
    uint8_t codeHigh = chStr[i];
    uint8_t codeLow = chStr[i + 1];

    long offset = ((codeHigh - 0xA1) * 94 + (codeLow - 0xA1)) * 32;

    uint8_t dotMatrix[32];
    fontFile.seek(offset);
    fontFile.read(dotMatrix, 32);
    for (int row = 0; row < 16; row++) {
      uint8_t dotHigh = dotMatrix[row * 2];
      uint8_t dotLow = dotMatrix[row * 2 + 1];

      for (int col = 0; col < 16; col++) {

        int charOffsetX = (i / 2) * 16;
        int charOffsetY = 0;

        uint16_t pixelX = x + charOffsetX + col, pixelY = y + charOffsetY + row;

        if (pixelX >= TFT_HEIGHT || pixelY >= TFT_WIDTH)
          continue;

        bool isLight = false;
        if (col < 8) {
          isLight = (dotHigh & (0x80 >> col)) != 0;
        } else {
          isLight = (dotLow & (0x80 >> (col - 8))) != 0;
        }
        if (isLight) {
          tft->drawPixel(pixelX, pixelY, color);
        } else {
          if (!noneBg) {
            tft->drawPixel(pixelX, pixelY, bgcolor);
          }
        }
      }
    }
  }
}