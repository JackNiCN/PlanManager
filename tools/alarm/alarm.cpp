#include "alarm.h"

Alarm alarm;


Alarm::Alarm()
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

void Alarm::start_alarm() {
  if (!is_alarming) { 
    is_alarming = true;
    alarm_start_time = millis();
  }
}

void Alarm::buzzer_alarm_task(void *parameter) {
  // 任务循环
  while (true) {
    if (is_alarming) {
      if (millis() - alarm_start_time >= ALARM_DURATION) {
        is_alarming = false;
        digitalWrite(BUZZER_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
        vTaskDelay(BUZZER_ON_TIME / portTICK_PERIOD_MS); // 响指定时长
        digitalWrite(BUZZER_PIN, HIGH);
        vTaskDelay(BUZZER_OFF_TIME / portTICK_PERIOD_MS); // 停指定时长
      }
    } else {
      // 无报警时，蜂鸣器关闭
      digitalWrite(BUZZER_PIN, HIGH);
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}