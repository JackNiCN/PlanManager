#ifndef TFTMENU_H
#define TFTMENU_H
#if ARDUINO > 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <TFT_eSPI.h>
#include <Debuger.h>

#include <SplitChinese.h>

namespace TFTMenuStruct
{
    struct MenuItem
    {
        bool (*drawStringFunction)(SplitChinese::TextUnit*, int, int);
        SplitChinese::TextUnit* textBuffer;
        int bufferCount;
    };
}

class TFTMenu
{
private:
    TFT_eSPI *tft;
    TFTMenu() = delete;

public:
    TFTMenu(TFT_eSPI *tftInstance);
};
#endif