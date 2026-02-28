#include <TFTMenu.h>


// create internal sprite covering the full display
TFTMenu::TFTMenu(TFT_eSPI *tftInstance, int maxItemCount) : tft(tftInstance), maxItems(maxItemCount), pSpr(nullptr)
{
    if (tft) {
        pSpr = new TFT_eSprite(tft);
        pSpr->createSprite(tft->width(), tft->height());
        ownsSprite = true;
    }
    itemList = new String[maxItems];
    itemCount = 0;
    currentItem = 0;
    Debug.Info("TFTMenu initialized");
}

// use an externally supplied sprite (caller responsible for creating)
TFTMenu::TFTMenu(TFT_eSPI *tftInstance, TFT_eSprite* sprite, int maxItemCount)
    : tft(tftInstance), pSpr(sprite), maxItems(maxItemCount), ownsSprite(false)
{
    itemList = new String[maxItems];
    itemCount = 0;
    currentItem = 0;
    Debug.Info("TFTMenu initialized with external sprite");
}

TFTMenu::~TFTMenu(){
    delete[] itemList;
    if (ownsSprite && pSpr) {
        pSpr->deleteSprite();
        delete pSpr;
    }
}

void TFTMenu::setSprite(TFT_eSprite* sprite, bool takeOwnership) {
    if (ownsSprite && pSpr) {
        pSpr->deleteSprite();
        delete pSpr;
    }
    pSpr = sprite;
    ownsSprite = takeOwnership;
}

void TFTMenu::setWindowPosition(int _x, int _y, int _w, int _h){
    x = _x;
    y = _y;
    width = _w;
    height = _h;
    // no action needed for sprite size since we usually use full-screen sprite
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
}


bool TFTMenu::showMenu(int pageIndex, uint32_t color, uint32_t bgColor){
    if (pSpr == nullptr) {
        Debug.Error("TFT instance is null!");
        return false;
    }

    pSpr->fillSprite(bgColor);

    int32_t itemsPerPage = height / 20; 
    if(itemsPerPage < 1){
        Debug.Error("Menu height too small!");
        return false;
    }


    pageCount = (itemCount + itemsPerPage - 1) / itemsPerPage;

    if (pageIndex < 1) pageIndex = 1;
    if (pageIndex != 1 && pageIndex > pageCount) pageIndex = pageCount;

    Debug.Info("show menu, page: %d, total pages: %d", pageIndex, pageCount);


    pSpr->fillRect(x, y, width, height, bgColor);
    pSpr->drawRect(x, y, width, height, color);


    for(int i = 0; i < itemsPerPage; i++){
        pSpr->drawLine(x, y + 20 * i, x + width, y + 20 * i, color);
    }

    File fontFile = SD.open("/HZK16", FILE_READ);
    if(!fontFile){
        Debug.Error("Failed to open font file /HZK16!");
        return false;
    }

    // configure TextWrite to draw into the sprite
    Text.setSprite(pSpr);
    Text.setTFTClass(nullptr);

    int startIndex = (pageIndex - 1) * itemsPerPage;
    for(int i = startIndex, j = 0; j < itemsPerPage; i++, j++){
        int itemY = y + 20 * j + 2;
        if(currentItem == j){
            pSpr->fillRect(x+1, y + 20 * j + 1, width - 2, 19, color);
            if(i < itemCount){
                Text.WriteText(fontFile, itemList[i], x + 2, itemY, bgColor);
            }
        }else{
            if(i < itemCount){
                Text.WriteText(fontFile, itemList[i], x + 2, itemY, color, false, bgColor);
            }
        }
    }

    fontFile.close();
    pSpr->pushSprite(0, 0);
    // restore TextWrite for direct TFT drawing
    Text.setSprite(nullptr);
    Text.setTFTClass(tft);
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