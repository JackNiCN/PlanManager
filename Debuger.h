#ifndef DEBUGER_H
#define DEBUGER_H
#include<Arduino.h>
class Debuger{
private:
public:
    Debuger(){
        Serial.begin(115200);
    }
    void Info(String data, bool noEndline = false){
        if(noEndline){
            Serial.print("INFO: "+data);
        }else{
            Serial.println("INFO: "+data);
        }
    }
    void Debug(String data, bool noEndline = false){
        if(noEndline){
            Serial.print("Debug: "+data);
        }else{
            Serial.println("Debug: "+data);
        }
    }
    void Error(String data, bool noEndline = false){
        if(noEndline){
            Serial.print("Error: "+data);
        }else{
            Serial.println("Error: "+data);
        }
    }
    void Warning(String data, bool noEndline = false){
        if(noEndline){
            Serial.print("Warning: "+data);
        }else{
            Serial.println("Warning: "+data);
        }
    }
};
extern Debuger Debug;
#endif