#include "alarm.h"

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