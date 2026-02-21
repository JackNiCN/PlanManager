#include <TFTMenu.h>

TFTMenu::TFTMenu(TFT_eSPI *tftInstance, int maxItemCount) : tft(tftInstance)
{
    itemList = new String[maxItemCount];
    Debug.Info("TFTMenu initialized");
}

TFTMenu::~TFTMenu(){
    delete[] itemList;
}

void TFTMenu::setWindowPosition(int _x, int _y, int _w, int _h){
    x = _x;
    y = _y;
    width = _w;
    height = _h;
}

void TFTMenu::addItem(String item){
    itemList[itemCount++] = item;
}

void TFTMenu::clearItemList() {
    itemCount = 0;
}

bool TFTMenu::showMenu(int pageIndex, uint32_t color, uint32_t bgColor){
    if (tft == nullptr) {
        Debug.Error("TFT instance is null!");
        return false;
    }
    Debug.Info("show menu");
    tft->fillRect(x, y, width, height, bgColor);
    tft->drawRect(x, y, width, height, color);
    int32_t itemsPerPage = height / 20; 
    if(itemsPerPage < 1){
        return false;
    }
    for(int i = 0; i < itemsPerPage; i++){
        tft->drawLine(x, y + 20 * i, x + width, y + 20 * i, color);
    }
    pageCount = itemCount / itemsPerPage;
    for(int i = (pageIndex - 1) * itemsPerPage, j = 0; i < pageIndex * itemsPerPage; i++,j++){
        Text.setTFTClass(tft);
        File fontFile = SD.open("/HZK16", FILE_READ);
        if(!fontFile){
            return false;
        }
        if(currentItem != j){
            Text.WriteText(fontFile, itemList[i], x + 2, y + 20 * j + 2, color, false, bgColor);
        }else{
            tft->fillRect(x+1, y + 20 * j + 1, width - 2, 19, color);
            Text.WriteText(fontFile, itemList[i], x + 2, y + 20 * j + 2, bgColor);
        }
    }
    return true;
}

void TFTMenu::itemUp(){
    int32_t itemsPerPage = height / 20;
    currentItem = (currentItem + 1) % itemsPerPage;
}

void TFTMenu::itemDown(){
    int32_t itemsPerPage = height / 20;
    currentItem = (currentItem - 1 + itemsPerPage) % itemsPerPage;
}