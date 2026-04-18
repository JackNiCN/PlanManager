class UIManager
{
private:
    TFT_eSPI tft;
    TFT_eSprite spr(&tft);
   
    TFTMenu menu(&tft, &spr, 50);

public:
}