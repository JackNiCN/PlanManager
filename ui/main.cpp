#include "main.h"


void Main::doRenderMain()
{
    if (!config->buttonQueue.isEmpty())
    {
        config->alarm.is_alarming = false;
        if (config->buttonQueue.pop() == ButtonName::EXT_BUTTON)
        {
            sysState = SystemState::Menu;
            buttonQueue.clear();
            MenuChanged = true;
        }
    }

    Text.setTFTClass(&tft);
    Text.setSprite(&spr);
    if (isFirstRenderMainScreen)
    {
        spr.fillSprite(TFT_BLACK);
        File file = openSDFile("/HZK16");
        if (file)
        {
            Text.displayChinese(file, 2, 24, GB.get("当前计划："), TFT_WHITE, false, TFT_BLACK);
            file.close();
            spr.drawLine(0, 22, 160, 22, TFT_WHITE);
        }
        else
        {
            spr.setTextColor(TFT_RED, TFT_BLACK);
            spr.drawString("Connot open HZK16", 5, 24);
        }
        isFirstRenderMainScreen = false;
        file.close();
    }
    String time;
    time_t timestamp = rtc.getEpoch();
    struct tm *tmpPtr = localtime(&timestamp);
    struct tm timeinfo;
    if (tmpPtr)
        memcpy(&timeinfo, tmpPtr, sizeof(struct tm));
    else
        memset(&timeinfo, 0, sizeof(struct tm));
    time = (timeinfo.tm_hour < 10 ? "0" + String(timeinfo.tm_hour) : String(timeinfo.tm_hour)) + ":" + (timeinfo.tm_min < 10 ? "0" + String(timeinfo.tm_min) : String(timeinfo.tm_min));
    if (time != lastTimeStr)
    {
        lastTimeStr = time;
        spr.setTextSize(2);
        spr.setTextColor(TFT_WHITE, TFT_BLACK);
        spr.drawString(time, 1, 1);

        PlanItem plan = FindPlanbyTime(timestamp);
        static PlanItem lastFindPlan;
        lastFindPlan = plan;
        spr.fillRect(0, 40, 160, 80, TFT_BLACK);
        spr.fillRect(86, 24, 80, 16, TFT_BLACK);
        if (plan.name == "fail")
        {
            Debug.Warning("Get current plan faild");
        }
        else if (plan.name == "unknow")
        {
            File file = openSDFile("/HZK16");
            if (file)
            {
                Text.WriteText(file, "没有计划", 86, 24, TFT_WHITE);
            }
            else
            {
                Debug.Warning("File open Error");
            }
            file.close();
        }
        else
        {
            {
                File file = openSDFile("/HZK16");
                if (file)
                {
                    Text.WriteText(file, plan.name, 0, 40, TFT_WHITE);
                }
                else
                {
                    Debug.Warning("File open Error");
                }
                file.close();
            }
            struct tm STbuf;
            struct tm ETbuf;

            tmpPtr = localtime(&plan.startTime);
            if (tmpPtr)
                memcpy(&STbuf, tmpPtr, sizeof(struct tm));
            else
                memset(&STbuf, 0, sizeof(struct tm));

            tmpPtr = localtime(&plan.endTime);
            if (tmpPtr)
                memcpy(&ETbuf, tmpPtr, sizeof(struct tm));
            else
                memset(&ETbuf, 0, sizeof(struct tm));

            spr.setTextSize(1);
            String timeStr = (STbuf.tm_hour < 10 ? "0" + String(STbuf.tm_hour) : String(STbuf.tm_hour)) + ":" + (STbuf.tm_min < 10 ? "0" + String(STbuf.tm_min) : String(STbuf.tm_min));
            timeStr += " - ";
            timeStr += (ETbuf.tm_hour < 10 ? "0" + String(ETbuf.tm_hour) : String(ETbuf.tm_hour)) + ":" + (ETbuf.tm_min < 10 ? "0" + String(ETbuf.tm_min) : String(ETbuf.tm_min));
            spr.drawString(timeStr, 1, 60);
            spr.fillRect(1, 76, 16, 159, TFT_BLACK);
            File file = openSDFile("/HZK16");
            if (file)
            {
                Text.WriteText(file, "时长:" + String(plan.durationMinutes), 1, 76, TFT_WHITE);
            }
            else
            {
                Debug.Warning("File open Error");
            }
            file.close();
            if (STbuf.tm_hour == timeinfo.tm_hour && STbuf.tm_min == timeinfo.tm_min)
            {
                config.alarm.start_alarm();
            }
        }
    }
    if (millis() - lastRSSIDraw > 10000 || lastRSSIDraw == 0)
    {
        drawWiFiIcon(15, 160 - 12, getSignalLevel(WiFi.RSSI()));
        Debug.Debug(String(WiFi.RSSI()));
        lastRSSIDraw = millis();
    }
    spr.pushSprite(0, 0);
}
