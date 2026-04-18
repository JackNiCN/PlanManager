#ifdef ALARM_H
#define ALARM_H

#define BUZZER_PIN 22

class Alarm
{
private:
    /* data */
public:
    bool is_alarming;                   // 报警总开关
    unsigned long alarm_start_time;         // 报警启动时间戳
    const unsigned long ALARM_DURATION; // 报警持续时间（30秒）
    const int BUZZER_ON_TIME;             // 蜂鸣器响的时长（毫秒）
    const int BUZZER_OFF_TIME;            // 蜂鸣器停的时长（毫秒）

    void start_alarm();
    void buzzer_alarm_task(void *parameter);

    Alarm(/* args */);
    ~Alarm();
};

Alarm::Alarm(/* args */)
{
    is_alarming = false;                   // 报警总开关
    alarm_start_time = 0;         // 报警启动时间戳
    ALARM_DURATION = 30000; // 报警持续时间（30秒）
    BUZZER_ON_TIME = 300;             // 蜂鸣器响的时长（毫秒）
    BUZZER_OFF_TIME = 700;            // 蜂鸣器停的时长（毫秒）
}

Alarm::~Alarm()
{
}

#endif