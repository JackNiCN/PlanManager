#ifndef TEXTWRITE_H
#define TEXTWRITE_H

#if ARDUINO > 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <TFT_eSPI.h>
#include "SplitChinese.h"
#include "Debuger.h"
#include <SD.h>
#include <UTF8ToGB2312.h>

class TextWrite{
private:
    SplitChinese::SplitChinese spliter; 
    TFT_eSPI *tft;
    TFT_eSprite *sprite;

public:
    TextWrite();
    void setTFTClass(TFT_eSPI *_tft);
    void setSprite(TFT_eSprite *_sprite);
    void WriteText(File& fontFile, String text, int x, int y, uint16_t color, bool noneBg = true, uint16_t bgcolor = TFT_BLACK);
    void displayChinese(File &fontFile, int x, int y, String chStr, uint16_t color, bool noneBg, uint16_t bgcolor);
};
extern TextWrite Text;
#endif