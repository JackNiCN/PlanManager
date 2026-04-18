#ifndef Tools_SD_H
#define Tools_SD_H

#if ARDUINO > 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <TFT_eSPI.h>
#include "Debuger.h"
#include <SD.h>

class Sd {
private:

public:
    Sd();
    void setupSD(TFT_eSPI tft);
    File openSDFile(String fileName, TFT_eSPI tft);
};
#endif