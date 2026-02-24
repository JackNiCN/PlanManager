#include <TFTMenu.h>


TFTMenu::TFTMenu(TFT_eSPI *tftInstance, int maxItemCount) : tft(tftInstance), maxItems(maxItemCount)
{
    itemList = new String[maxItems];
    itemCount = 0;
    currentItem = 0;
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
    if (itemCount < maxItems) {
        itemList[itemCount++] = item;
    } else {
        Debug.Error("Menu item count exceeds max limit!");
    }
}


void TFTMenu::clearItemList() {

    for (int i = 0; i < itemCount; i++) {
        itemList[i] = "";
    }
    itemCount = 0;
    currentItem = 0;
}


bool TFTMenu::showMenu(int pageIndex, uint32_t color, uint32_t bgColor){
    if (tft == nullptr) {
        Debug.Error("TFT instance is null!");
        return false;
    }


    int32_t itemsPerPage = height / 20; 
    if(itemsPerPage < 1){
        Debug.Error("Menu height too small!");
        return false;
    }


    pageCount = (itemCount + itemsPerPage - 1) / itemsPerPage;

    if (pageIndex < 1) pageIndex = 1;
    if (pageIndex > pageCount) pageIndex = pageCount;

    Debug.Info("show menu, page: %d, total pages: %d", pageIndex, pageCount);


    tft->fillRect(x, y, width, height, bgColor);
    tft->drawRect(x, y, width, height, color);


    for(int i = 0; i < itemsPerPage; i++){
        tft->drawLine(x, y + 20 * i, x + width, y + 20 * i, color);
    }

    File fontFile = SD.open("/HZK16", FILE_READ);
    if(!fontFile){
        Debug.Error("Failed to open font file /HZK16!");
        return false;
    }

    Text.setTFTClass(tft);

    int startIndex = (pageIndex - 1) * itemsPerPage;
    for(int i = startIndex, j = 0; i < itemCount && j < itemsPerPage; i++, j++){
        int itemY = y + 20 * j + 2;
        if(currentItem == j){
            tft->fillRect(x+1, y + 20 * j + 1, width - 2, 19, color);
            Text.WriteText(fontFile, itemList[i], x + 2, itemY, bgColor);
        }else{
            Text.WriteText(fontFile, itemList[i], x + 2, itemY, color, false, bgColor);
        }
    }

    fontFile.close();
    return true;
}

void TFTMenu::itemUp(){
    int32_t itemsPerPage = height / 20;
    if(itemsPerPage < 1) return;
    currentItem = (currentItem - 1 + itemsPerPage) % itemsPerPage;
}

void TFTMenu::itemDown(){
    int32_t itemsPerPage = height / 20;
    if(itemsPerPage < 1) return;
    currentItem =  (currentItem + 1) % itemsPerPage;
}