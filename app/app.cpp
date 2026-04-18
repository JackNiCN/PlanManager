#include "app.h"

void App::init()
{
    Serial.begin(115200);
    delay(1000);
    Debug.Info("system start");
    sysState = SystemState::Initialization;
    setupTFT();
    setupSD();
    showSetupScreen();

    pinMode(BUZZER_PIN, OUTPUT);

    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(1000);
    digitalWrite(22, HIGH);

    xTaskCreate(
        alarm.buzzer_alarm_task, // 任务函数
        "BuzzerAlarm",     // 任务名称
        2048,              // 栈大小
        NULL,              // 传递给任务的参数
        2,                 // 任务优先级
        NULL               // 任务句柄
    );

    setupWifi();
    setupOTA();
    syncNTPTime();
    setupWebServer();
    preSortPlanList();
    menu.setWindowPosition(0, 0, 160, 128);
    sysState = SystemState::Normal;
}

void App::loop()
{
    if (millis() - lastNtpSync > NTP_SYNC_INTERVAL)
    {
        syncNTPTime();
        lastNtpSync = millis();
    }
    ArduinoOTA.handle();
    keyboardLoop();
    renderTFT();
    delay(100);
}

/// private
void App::setupTFT()
{
    tft.init();
    tft.setSwapBytes(true);
    tft.setRotation(1);
    spr.createSprite(160, 128);
    Text.setTFTClass(&tft);
    Text.setSprite(&spr);
    Debug.Info("TFT init over. ");
}

void App::showSetupScreen()
{
    spr.fillSprite(TFT_BLACK);
    File file = openSDFile("/HZK16");
    if (!file)
    {
        Debug.Error("read HZK16 ERROR");
    }
    Text.WriteText(file, "正在启动", 20, 20, TFT_WHITE);
    file.close();
    spr.pushSprite(0, 0);
}


void App::setupOTA()
{
    ArduinoOTA
        .onStart([]()
                 {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {
        type = "filesystem";
      }

      Serial.println("Start updating " + type);
      server.end(); })
        .onEnd([]()
               { Serial.println("\nEnd"); })
        .onProgress([](unsigned int progress, unsigned int total)
                    { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
        .onError([](ota_error_t error)
                 {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      } });

    ArduinoOTA.begin();
}

void App::keyboardLoop()
{
    int upAndDown = analogRead(35);
    if (upAndDown > 2048)
    {
        if (upButtonLastState == 0)
        {
            upButtonLastState = 1;
            Debug.Debug("Up Click");
            buttonQueue.push(ButtonName::UP_BUTTON);
        }
    }
    else if (upAndDown > 32 && upAndDown < 2048)
    {
        if (downButtonLastState == 0)
        {
            downButtonLastState = 1;
            Debug.Debug("Down Click");
            buttonQueue.push(ButtonName::DOWN_BUTTON);
        }
    }
    else
    {
        upButtonLastState = 0;
        downButtonLastState = 0;
    }
    int leftAndRight = analogRead(32);
    if (leftAndRight > 2048)
    {
        if (leftButtonLastState == 0)
        {
            leftButtonLastState = 1;
            Debug.Debug("Left Click");
            buttonQueue.push(ButtonName::LEFT_BUTTON);
        }
    }
    else if (leftAndRight > 32 && leftAndRight < 2048)
    {
        if (rightButtonLastState == 0)
        {
            rightButtonLastState = 1;
            Debug.Debug("Right Click");
            buttonQueue.push(ButtonName::RIGHT_BUTTON);
        }
    }
    else
    {
        leftButtonLastState = 0;
        rightButtonLastState = 0;
    }

    int midAndExt = analogRead(34);
    if (midAndExt > 2048)
    {
        if (middleButtonLastState == 0)
        {
            middleButtonLastState = 1;
            Debug.Debug("Middle Click");
            buttonQueue.push(ButtonName::MIDDLE_BUTTON);
        }
    }
    else if (midAndExt > 32 && midAndExt < 2048)
    {
        if (extButtonLastState == 0)
        {
            extButtonLastState = 1;
            Debug.Debug("Ext Click");
            buttonQueue.push(ButtonName::EXT_BUTTON);
        }
    }
    else
    {
        middleButtonLastState = 0;
        extButtonLastState = 0;
    }
}

bool App::createJson()
{
    String templateStr = "";
    File templateFile = SD.open("/template.json", FILE_READ);
    while (templateFile.available())
    {
        templateStr += templateFile.read();
    }
    templateFile.close();
    File file = SD.open("/plans.json", FILE_WRITE);
    file.write((const uint8_t *)templateStr.c_str(), templateStr.length());
    file.close();
    return true;
}

void App::renderTFT()
{
    if (sysState == SystemState::Normal)
    {
        doRenderMain();
    }
    else
    {
        isFirstRenderMainScreen = false;
    }
    if (sysState == SystemState::Screensave)
    {
        if (!buttonQueue.isEmpty())
        {
            buttonQueue.clear();
            sysState = SystemState::Normal;
            isFirstRenderMainScreen = true;
            lastPlanCheck = 0;
            lastRSSIDraw = 0;
            return;
        }
    }
    if (sysState == SystemState::Menu)
    {
        doRenderMenu();
    }
    if (sysState == SystemState::MoreInfo)
    {
        if (!buttonQueue.isEmpty())
        {
            if (buttonQueue.pop() == ButtonName::MIDDLE_BUTTON)
            {
                sysState = SystemState::Menu;
                buttonQueue.clear();
                MenuChanged = true;
            }
        }
    }
}

void App::doRenderScreenSave()
{
}

void App::doRenderMenu()
{

    if (!buttonQueue.isEmpty())
    {
        ButtonName currentButton = buttonQueue.pop();

        if (currentButton == ButtonName::EXT_BUTTON)
        {
            sysState = SystemState::Normal;
            isFirstRenderMainScreen = true;
            lastPlanCheck = 0;
            lastRSSIDraw = 0;
            lastTimeStr = "";
            buttonQueue.clear();
            promptTone();
        }
        if (currentButton == ButtonName::UP_BUTTON)
        {
            menu.itemUp();
            promptTone();
            MenuChanged = true;
        }
        if (currentButton == ButtonName::DOWN_BUTTON)
        {
            menu.itemDown();
            promptTone();
            MenuChanged = true;
        }
        if (currentButton == ButtonName::LEFT_BUTTON)
        {
            if (menuPageIndex > 1)
            {
                menuPageIndex--;
            }
            promptTone();
            MenuChanged = true;
        }
        if (currentButton == ButtonName::RIGHT_BUTTON)
        {
            menuPageIndex++;
            promptTone();
            MenuChanged = true;
        }
        if (currentButton == ButtonName::MIDDLE_BUTTON)
        {
            moreInfoPage(menu.getCurrentItem());
            sysState = SystemState::MoreInfo;
            return;
        }
    }
    if (MenuChanged)
    {
        menu.clearItemList();
        File jsonFile = SD.open("/plans.json", FILE_READ);
        if (!jsonFile)
        {
            Debug.Error("file not open.Connot find plan.");
        }

        size_t fileSize = jsonFile.size();
        DynamicJsonDocument doc(fileSize * 4);

        DeserializationError error1 = deserializeJson(doc, jsonFile);
        jsonFile.close();
        if (error1)
        {
            Debug.Error("解析JSON文件错误");
        }
        serializeJsonPretty(doc["planList"], Serial);
        Debug.Debug(String(doc["planList"].size()));
        for (int i = 0; i < doc["planList"].size(); i++)
        {
            String dateS = doc["planList"][i]["date"].as<String>();
            String bS = doc["planList"][i]["beginTime"].as<String>();
            String eS = doc["planList"][i]["endTime"].as<String>();
            time_t startTime = stringToTime(dateS, bS);
            time_t endTime = stringToTime(dateS, eS);

            long durationMinutes = doc["planList"][i]["durationMinutes"].as<long>();
            Serial.printf("PlanCheck:%ld-%ld curr:%ld\n", (long)startTime, (long)endTime, (long)time);
            menu.addItem({doc["planList"][i]["name"].as<String>(), startTime, durationMinutes, endTime, doc["planList"][i]["description"].as<String>(), doc["planList"][i]["id"].as<String>()});
        }

        menu.showMenu(menuPageIndex, TFT_BLUE, TFT_BLACK);
        MenuChanged = false;
    }
}

int App::getSignalLevel(int rssi)
{
    if (rssi >= -50)
        return 4;
    else if (rssi >= -60)
        return 3;
    else if (rssi >= -70)
        return 2;
    else if (rssi >= -80)
        return 1;
    else
        return 0;
}

void App::drawWiFiIcon(int x, int y, int level)
{
    spr.fillRect(y - 8, x - 10, 16, 16, TFT_BLACK);
    spr.fillCircle(y, x, 2, level > 0 ? TFT_BLUE : 0x39C4);
    if (level >= 1)
        spr.fillRect(y - 1, x - 2, 3, 2, TFT_BLUE);
    else if (level > 0)
        spr.drawRect(y - 1, x - 2, 3, 2, 0x39C4);
    if (level >= 2)
        spr.fillRect(y - 3, x - 5, 6, 2, TFT_BLUE);
    else if (level > 0)
        spr.drawRect(y - 3, x - 5, 6, 2, 0x39C4);
    if (level >= 3)
        spr.fillRect(y - 4, x - 8, 9, 2, TFT_BLUE);
    else if (level > 0)
        spr.drawRect(y - 4, x - 8, 9, 2, 0x39C4);
    if (level >= 4)
        spr.fillRect(y - 6, x - 11, 12, 2, TFT_BLUE);
    else if (level > 0)
        spr.drawRect(y - 6, x - 11, 12, 2, 0x39C4);
}

PlanItem App::FindPlanbyTime(time_t time)
{
    File jsonFile = SD.open("/plans.json", FILE_READ);
    if (!jsonFile)
    {
        Debug.Error("file not open.Connot find plan.");
        return {"fail", 0, 0, 0, "fail", "fail"};
    }

    size_t fileSize = jsonFile.size();
    DynamicJsonDocument doc(fileSize * 4);

    DeserializationError error1 = deserializeJson(doc, jsonFile);
    jsonFile.close();
    if (error1)
    {
        Debug.Error("解析JSON文件错误");
        return {"fail", 0, 0, 0, "fail", "fail"};
    }
    serializeJsonPretty(doc["planList"], Serial);
    Debug.Debug(String(doc["planList"].size()));
    for (int i = 0; i < doc["planList"].size(); i++)
    {
        String dateS = doc["planList"][i]["date"].as<String>();
        String bS = doc["planList"][i]["beginTime"].as<String>();
        String eS = doc["planList"][i]["endTime"].as<String>();
        Serial.printf("DEBUG plan[%d] date='%s' begin='%s' end='%s'\n", i, dateS.c_str(), bS.c_str(), eS.c_str());

        time_t startTime = stringToTime(dateS, bS);
        time_t endTime = stringToTime(dateS, eS);
        long durationMinutes = doc["planList"][i]["durationMinutes"].as<long>();
        Serial.printf("PlanCheck:%ld-%ld curr:%ld\n", (long)startTime, (long)endTime, (long)time);
        if (time >= startTime && time <= endTime)
        {
            return {doc["planList"][i]["name"].as<String>(), startTime, durationMinutes, endTime, doc["planList"][i]["description"].as<String>(), doc["planList"][i]["id"].as<String>()};
        }
    }

    return {"unknow", 0, 0, 0, "unknow", "unknow"};
}

time_t App::stringToTime(const String &dateStr, const String &timeStr)
{
    // 1. 解析日期字符串 YYYY-MM-DD
    int year = dateStr.substring(0, 4).toInt();
    int month = dateStr.substring(5, 7).toInt();
    int day = dateStr.substring(8, 10).toInt();

    // 2. 解析时间字符串 HH:MM
    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(3, 5).toInt();
    int second = 0; // 未提供秒数，默认设为0

    // 3. 校验输入格式的合法性（基础校验）
    if (dateStr.length() != 10 || timeStr.length() != 5 || dateStr.charAt(4) != '-' || dateStr.charAt(7) != '-' || timeStr.charAt(2) != ':' || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59)
    {
        Serial.println("错误：日期或时间格式非法！");
        return 0; // 返回0表示解析失败
    }

    // 4. 填充tm结构体（tm_mon从0开始，所以月份要-1；tm_year是从1900年开始的年数）
    struct tm timeInfo = {0};
    timeInfo.tm_sec = second;       // 秒
    timeInfo.tm_min = minute;       // 分
    timeInfo.tm_hour = hour;        // 时
    timeInfo.tm_mday = day;         // 日
    timeInfo.tm_mon = month - 1;    // 月（0-11）
    timeInfo.tm_year = year - 1900; // 年（从1900开始）
    timeInfo.tm_isdst = -1;         // 自动判断夏令时

    // 5. 转换为time_t时间戳
    time_t timestamp = mktime(&timeInfo);

    // 校验转换结果
    if (timestamp == (time_t)-1)
    {
        Serial.println("错误：时间转换失败（非法日期，如2月30日）！");
        return 0;
    }

    return timestamp;
}

void App::promptTone()
{
    digitalWrite(BUZZER_PIN, LOW);
    delay(150);
    digitalWrite(BUZZER_PIN, HIGH);
}

void App::quickSortJsonArray(JsonArray arr, int low, int high, bool (*comparator)(JsonVariant a, JsonVariant b))
{
    if (low >= high)
        return;

    // 选基准值 ——必须复制值，不能只是拿到引用
    // JsonVariant pivot = arr[high]; // 原来这只是引用, 后续交换会改变它
    DynamicJsonDocument pivotDoc(256);
    JsonVariant pivot = pivotDoc.to<JsonVariant>();
    pivot.set(arr[high]);

    int i = low - 1;

    // 分区
    for (int j = low; j < high; j++)
    {
        if (comparator(arr[j], pivot))
        {
            i++;
            // 交换arr[i]和arr[j]，使用临时 JsonVariant 深拷贝
            DynamicJsonDocument tmpDoc(256);
            JsonVariant temp = tmpDoc.to<JsonVariant>();
            temp.set(arr[i]);   // 保存原始 arr[i]
            arr[i].set(arr[j]); // arr[i] = arr[j]
            arr[j].set(temp);   // arr[j] = old arr[i]
        }
    }

    // 交换基准值到正确位置，同样使用深拷贝
    i++;
    {
        DynamicJsonDocument tmpDoc(256);
        JsonVariant temp = tmpDoc.to<JsonVariant>();
        temp.set(arr[i]);
        arr[i].set(arr[high]);
        arr[high].set(temp);
    }

    // 递归排序左右分区
    quickSortJsonArray(arr, low, i - 1, comparator);
    quickSortJsonArray(arr, i + 1, high, comparator);
}

// 对外封装的快速排序函数
void App::sortJsonArrayByVariantQuick(JsonArray jsonArray, bool (*comparator)(JsonVariant a, JsonVariant b))
{
    if (jsonArray.size() <= 1)
        return;
    quickSortJsonArray(jsonArray, 0, jsonArray.size() - 1, comparator);
}

bool App::planListComparator(JsonVariant a, JsonVariant b)
{
    String AdateS = a["date"].as<String>();
    String AbeginS = a["beginTime"].as<String>();
    time_t AstartTime = stringToTime(AdateS, AbeginS);
    String BdateS = b["date"].as<String>();
    String BbeginS = b["beginTime"].as<String>();
    time_t BstartTime = stringToTime(BdateS, BbeginS);
    return AstartTime < BstartTime;
}
/**
 * @brief 向已有序的JSON数组中插入新元素（保持数组有序）
 * @param sortedArray 已排序的JsonArray（升序/降序由comparator决定）
 * @param newElement  要插入的新元素（JsonVariant）
 * @param comparator  比较函数（a应排在b前则返回true，如升序：a<int>() < b<int>()）
 * @return int        新元素插入的索引位置
 */
int App::insertIntoSortedJsonArray(JsonArray sortedArray, JsonVariant newElement, bool (*comparator)(JsonVariant a, JsonVariant b))
{
    if (!preSortOK)
    {
        return -1;
    }
    size_t n = sortedArray.size();

    int insertIndex = 0;
    for (; insertIndex < n; insertIndex++)
    {
        if (!comparator(sortedArray[insertIndex], newElement))
        {
            break;
        }
    }

    sortedArray.add(JsonVariant());

    for (int i = n; i > insertIndex; i--)
    {
        sortedArray[i].set(sortedArray[i - 1]);
    }

    sortedArray[insertIndex].set(newElement);

    return insertIndex;
}

void App::preSortPlanList()
{
    File jsonFile = SD.open("/plans.json", FILE_READ);
    if (!jsonFile)
    {
        Debug.Warning("file not open.All Sort Will Not Be used.");
        return;
    }

    size_t fileSize = jsonFile.size();

    DynamicJsonDocument fileDoc(fileSize * 2);

    DeserializationError error1 = deserializeJson(fileDoc, jsonFile);
    jsonFile.close();
    if (error1)
    {
        Debug.Warning("deserializeJson file faild.All Sort Will Not Be used.");
        return;
    }
    sortJsonArrayByVariantQuick(fileDoc["planList"], planListComparator);
    jsonFile = SD.open("/plans.json", FILE_WRITE);
    if (!jsonFile)
    {
        Debug.Warning("file not open.");
        return;
    }
    serializeJsonPretty(fileDoc, jsonFile);
    jsonFile.close();
    preSortOK = true;
}

void App::moreInfoPage(PlanItem plan)
{
    spr.fillSprite(TFT_BLACK);
    Text.setSprite(&spr);
    File fontFile = openSDFile("/HZK16");
    Text.WriteText(fontFile, "名称：", 3, 3, TFT_WHITE);
    Text.WriteText(fontFile, plan.name, 52, 3, TFT_WHITE);
    Text.WriteText(fontFile, "开始时间：", 3, 20, TFT_WHITE);
    {
        String time;
        time_t timestamp = plan.startTime;
        struct tm *tmpPtr = localtime(&timestamp);
        struct tm timeinfo;
        if (tmpPtr)
            memcpy(&timeinfo, tmpPtr, sizeof(struct tm));
        else
            memset(&timeinfo, 0, sizeof(struct tm));
        time = (timeinfo.tm_hour < 10 ? "0" + String(timeinfo.tm_hour) : String(timeinfo.tm_hour)) + ":" + (timeinfo.tm_min < 10 ? "0" + String(timeinfo.tm_min) : String(timeinfo.tm_min));
        Text.WriteText(fontFile, time, 84, 20, TFT_WHITE);
    }
    Text.WriteText(fontFile, "结束时间：", 3, 40, TFT_WHITE);
    {
        String time;
        time_t timestamp = plan.endTime;
        struct tm *tmpPtr = localtime(&timestamp);
        struct tm timeinfo;
        if (tmpPtr)
            memcpy(&timeinfo, tmpPtr, sizeof(struct tm));
        else
            memset(&timeinfo, 0, sizeof(struct tm));
        time = (timeinfo.tm_hour < 10 ? "0" + String(timeinfo.tm_hour) : String(timeinfo.tm_hour)) + ":" + (timeinfo.tm_min < 10 ? "0" + String(timeinfo.tm_min) : String(timeinfo.tm_min));
        Text.WriteText(fontFile, time, 84, 40, TFT_WHITE);
    }
    Text.WriteText(fontFile, "备注：", 3, 60, TFT_WHITE);
    Text.WriteText(fontFile, plan.info, 51, 60, TFT_WHITE);
    spr.pushSprite(0, 0);
}