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

namespace TFTMenuStruct {
struct MenuItem {
  bool (*drawStringFunction)(SplitChinese::TextUnit*, int, int);
  SplitChinese::TextUnit* textBuffer;
  int bufferCount;
  ~MenuItem() {
    delete[] this->textBuffer;
    this->textBuffer = nullptr;
  }

  MenuItem(){
  }

  MenuItem(const MenuItem& other) {
    this->drawStringFunction = other.drawStringFunction;
    this->bufferCount = other.bufferCount;
    this->textBuffer = new SplitChinese::TextUnit[other.bufferCount];
    for (int i = 0; i < other.bufferCount; i++) {
      this->textBuffer[i] = other.textBuffer[i];
    }
  }
  // 重载赋值运算符
  MenuItem& operator=(const MenuItem& other) {
    if (this == &other) return *this;
    delete[] this->textBuffer;
    this->drawStringFunction = other.drawStringFunction;
    this->bufferCount = other.bufferCount;
    this->textBuffer = new SplitChinese::TextUnit[other.bufferCount];
    for (int i = 0; i < other.bufferCount; i++) {
      this->textBuffer[i] = other.textBuffer[i];
    }
    return *this;
  }
};
}

class TFTMenu {
private:
  TFT_eSPI* tft;
  TFTMenu() = delete;
  TFTMenuStruct::MenuItem* itemList;
  int itemCount = 0;
public:
  TFTMenu(TFT_eSPI* tftInstance, int maxItemCount = 40);
  ~TFTMenu();
  void addItem(TFTMenuStruct::MenuItem&& item);
  bool showMenu(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t bgColor);
};
#endif