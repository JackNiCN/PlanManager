#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "config/config.h"

class Network
{
private:
    ESP32Time rtc;
    AsyncWebServer server(80);
    File uploadFile;
public:
    void setupWifi();
    void syncNTPTime();
    void setupWebServer();
}

#endif