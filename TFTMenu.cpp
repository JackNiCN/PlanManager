#include <TFTMenu.h>

TFTMenu::TFTMenu(TFT_eSPI *tftInstance, int maxItemCount) : tft(tftInstance)
{
    itemList = new TFTMenuStruct::MenuItem[maxItemCount];
    Debug.Info("TFTMenu initialized");
}

TFTMenu::~TFTMenu(){
    //delete[] itemList;
}

void TFTMenu::addItem(TFTMenuStruct::MenuItem&& item){
    itemList[itemCount] = item;
}

bool TFTMenu::showMenu(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t bgColor){
    if (tft == nullptr) {
        Debug.Error("TFT instance is null!");
        return false;
    }
    Debug.Info("show menu");
    tft->fillRect(x, y, w, h, bgColor);
    Debug.Debug("fill rect");
    tft->drawRect(x, y, w, h, color);
    Debug.Debug("draw Rect");
    delay(5000);
    int32_t itemsPerPage = h / 20; 
    if(itemsPerPage < 1){
        return false;
    }
    for(int i = 0; i < itemsPerPage; i++){
        tft->drawLine(x, y + 20 * i, x + w, y + 20 * i, color);
    }
    return true;
}