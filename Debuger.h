#ifndef DEBUGER_H
#define DEBUGER_H
#include <Arduino.h>
#include <stdarg.h>  // 必须包含可变参数列表的头文件

class Debuger {
private:
    void printLog(const char* level, const char* format, va_list args) {
        Serial.print(level);
        Serial.print(": ");
        
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        Serial.println(buffer);
    }

public:
    Debuger() {
        Serial.begin(115200);
    }

    void Info(const char* format, ...) {
        va_list args;
        va_start(args, format);
        printLog("INFO", format, args);
        va_end(args);
    }

    void Debug(const char* format, ...) {
        va_list args;
        va_start(args, format);
        printLog("Debug", format, args);
        va_end(args);
    }

    void Error(const char* format, ...) {
        va_list args;
        va_start(args, format);
        printLog("Error", format, args);
        va_end(args);
    }

    void Warning(const char* format, ...) {
        va_list args;
        va_start(args, format);
        printLog("Warning", format, args);
        va_end(args);
    }

    void Info(String data, bool noEndline = false) {
        if (noEndline) {
            Serial.print("INFO: " + data);
        } else {
            Serial.println("INFO: " + data);
        }
    }

    void Debug(String data, bool noEndline = false) {
        if (noEndline) {
            Serial.print("Debug: " + data);
        } else {
            Serial.println("Debug: " + data);
        }
    }

    void Error(String data, bool noEndline = false) {
        if (noEndline) {
            Serial.print("Error: " + data);
        } else {
            Serial.println("Error: " + data);
        }
    }

    void Warning(String data, bool noEndline = false) {
        if (noEndline) {
            Serial.print("Warning: " + data);
        } else {
            Serial.println("Warning: " + data);
        }
    }
};

extern Debuger Debug;

#endif