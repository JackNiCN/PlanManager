#ifndef ALARM_H
#define ALARM_H



class Alarm
{
private:
    /* data */
public:
    bool is_alarming;                   // 报警总开关
    unsigned long alarm_start_time;         // 报警启动时间戳
    unsigned long ALARM_DURATION; // 报警持续时间（30秒）
    int BUZZER_ON_TIME;             // 蜂鸣器响的时长（毫秒）
    int BUZZER_OFF_TIME;            // 蜂鸣器停的时长（毫秒）

    void start_alarm();
    void buzzer_alarm_task(void *parameter);

    Alarm();
    ~Alarm();
};


extern Alarm alarm;

#endif