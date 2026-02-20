#include <TFTMenu.h>

TFTMenu::TFTMenu(TFT_eSPI *tftInstance, int maxItemCount) : tft(tftInstance)
{
    itemList = new String[maxItemCount];
    Debug.Info("TFTMenu initialized");
}

TFTMenu::~TFTMenu(){
    delete[] itemList;
}

void TFTMenu::addItem(String item){
    itemList[itemCount++] = item;
}

void TFTMenu::clearItemList() {
    itemCount = 0;
}

bool TFTMenu::showMenu(int32_t x, int32_t y, int32_t w, int32_t h, int pageIndex, uint32_t color, uint32_t bgColor){
    if (tft == nullptr) {
        Debug.Error("TFT instance is null!");
        return false;
    }
    height = h;
    width = w;
    Debug.Info("show menu");
    tft->fillRect(x, y, w, h, bgColor);
    tft->drawRect(x, y, w, h, color);
    int32_t itemsPerPage = h / 20; 
    if(itemsPerPage < 1){
        return false;
    }
    for(int i = 0; i < itemsPerPage; i++){
        tft->drawLine(x, y + 20 * i, x + w, y + 20 * i, color);
    }
    pageCount = itemCount / itemsPerPage;
    for(int i = (pageIndex - 1) * itemsPerPage, j = 0; i < pageIndex * itemsPerPage; i++,j++){
        Text.setTFTClass(tft);
        File fontFile = SD.open("/HZK16", FILE_READ);
        if(!fontFile){
            return false;
        }
        Text.WriteText(fontFile, itemList[i], x + 2, y + 20 * i + 2, color, false, bgColor);
    }
    return true;
}