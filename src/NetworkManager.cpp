#include "NetworkManager.h"

NetManager::NetManager(const char* ssid, const char* pass,
                    IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns)
                    : _ssid(ssid), _pass(pass), _ip(ip), _gateway(gateway), _subnet(subnet), _dns(dns) 
                    {}

void NetManager::connect() {
    // setting up the station mode
    WiFi.mode(WIFI_STA);

    if (!WiFi.config(_ip, _gateway, _subnet, _dns)) {
        LOG("[Net] Static IP Cofnig error!");
    }

    WiFi.begin(_ssid, _pass);
    LOGF("[Net] Connecting to ", _ssid);

    unsigned long startAttempt = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
        delay(500);
        LOG(".");
    }

    LOG("");

    if (WiFi.status() == WL_CONNECTED) {
        LOG("[Net] Connected!");
        LOGF("[Net] IP: ", WiFi.localIP());
    }
}

bool NetManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}