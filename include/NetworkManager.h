#pragma once
#include "config.h"
#include "secrets.h"

#include <Arduino.h>
#include <WiFi.h>

class NetManager {
private:
    const char* _ssid;
    const char* _pass;
    IPAddress _ip;
    IPAddress _gateway;
    IPAddress _subnet;
    IPAddress _dns;

public:
    NetManager(const char* ssid, const char* pass,
               IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns);

    void connect(); // initialize the connection

    bool isConnected();
    IPAddress getLocalIP();
};