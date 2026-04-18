#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "config/config.h"

class Network
{
private:
  Config config;
public:
    void setupWifi();
    void syncNTPTime();
    void setupWebServer();
}

#endif