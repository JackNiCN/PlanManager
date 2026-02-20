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
#include <TextWrite.h>


class TFTMenu {
private:
  TFT_eSPI* tft;
  TFTMenu() = delete;
  String* itemList;
  int itemCount = 0;
  int pageCount = 0;
  int height = 0;
  int width = 0;
public:
  TFTMenu(TFT_eSPI* tftInstance, int maxItemCount = 40);
  ~TFTMenu();
  void addItem(String item);
  void clearItemList();
  int getPageCount() const{
    return pageCount;
  }
  bool showMenu(int32_t x, int32_t y, int32_t w, int32_t h, int pageIndex, uint32_t color, uint32_t bgColor);
};
#endif