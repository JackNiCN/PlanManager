#include <TFTMenu.h>

TFTMenu::TFTMenu(TFT_eSPI *tftInstance) : tft(tftInstance)
{
    Debug.Info("TFTMenu initialized");
}
