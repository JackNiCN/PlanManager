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
#include <SD.h>

class TFTMenu {
private:
  TFT_eSPI* tft;
  TFTMenu() = delete;
  String* itemList;
  int itemCount = 0;
  int pageCount = 0;
  int height = 0;
  int width = 0;
  int currentItem = 0;
  int x,y;
  int maxItems;
public:
  TFTMenu(TFT_eSPI* tftInstance, int maxItemCount = 40);
  ~TFTMenu();
  void setWindowPosition(int _x, int _y, int _w, int _h);
  void addItem(String item);
  void clearItemList();
  int getPageCount() const{
    return pageCount;
  }
  void itemUp();
  void itemDown();
  bool showMenu(int pageIndex, uint32_t color, uint32_t bgColor);
};
#endif