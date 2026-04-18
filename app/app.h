#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include "../config/config.h"
#include "../input/input.h"
#include "../network/network.h"
#include "../tools/alarm/alarm.h"
#include "../tools/sd/sd.h"
#include "../tools/text/text.h"
#include "../ui/info.h"
#include "../ui/main.h"
#include "../ui/menu.h"
#include "../ui/UIManager.h"


class App
{
private:
    SystemState sysState;
    String lastTimeStr;

    void setupTFT();
    void showSetupScreen();
    void setupWifi();
    void setupSD();
    void syncNTPTime();
    void setupWebServer();
    void setupOTA();
    void keyboardLoop();
    File openSDFile(String fileName);
    bool createJson();

    bool isFirstRenderMainScreen = true;
    bool MenuChanged = true;
    unsigned long lastRSSIDraw = 0;
    unsigned long lastPlanCheck = 0;

    void renderTFT();
    void doRenderMain();
    void doRenderScreenSave();
    void doRenderMenu();
    int getSignalLevel(int rssi);
    void drawWiFiIcon(int x, int y, int level);
    PlanItem FindPlanbyTime(time_t);
    time_t stringToTime(const String &dateStr, const String &timeStr);
    void promptTone();
    void quickSortJsonArray(JsonArray arr, int low, int high, bool (*comparator)(JsonVariant a, JsonVariant b));
    void sortJsonArrayByVariantQuick(JsonArray jsonArray, bool (*comparator)(JsonVariant a, JsonVariant b));
    bool planListComparator(JsonVariant a, JsonVariant b);
    int insertIntoSortedJsonArray(JsonArray sortedArray, JsonVariant newElement, bool (*comparator)(JsonVariant a, JsonVariant b));
    void preSortPlanList();
    void moreInfoPage(PlanItem plan);

public:
    void init();
    void loop();
}

#endif