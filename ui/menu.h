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
#include <TFTMenu.h>

struct PlanItem {
  String name;
  time_t startTime;
  long durationMinutes;
  time_t endTime;
  String info;
  String id;
};

class TFTMenu {
private:
  TFT_eSPI* tft;
  TFT_eSprite* pSpr;
  bool ownsSprite = false;
  TFTMenu() = delete;
  PlanItem* itemList;
  int itemCount = 0;
  int pageCount = 0;
  int height = 0;
  int width = 0;
  int currentItem = 0;
  int x,y;
  int maxItems;
public:
  // normal constructor creates an internal sprite sized to full display
  TFTMenu(TFT_eSPI* tftInstance, int maxItemCount = 40);
  // alternate constructor takes an existing sprite
  TFTMenu(TFT_eSPI* tftInstance, TFT_eSprite* sprite, int maxItemCount = 40);
  ~TFTMenu();

  // can set/change sprite later; take ownership if requested
  void setSprite(TFT_eSprite* sprite, bool takeOwnership = false);

  void setWindowPosition(int _x, int _y, int _w, int _h);
  void addItem(PlanItem item);
  void clearItemList();
  int getPageCount() const{
    return pageCount;
  }
  PlanItem getCurrentItem(int page){
    int32_t itemsPerPage = height / 20;
    return itemList[(page - 1) * itemsPerPage + currentItem];
  }
  void itemUp();
  void itemDown();
  bool showMenu(int pageIndex, uint32_t color, uint32_t bgColor);
};
#endif