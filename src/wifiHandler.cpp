#include "wifiHandler.h"
#include "logger.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <Ticker.h>

Ticker wifiConfigTicker;

// Timer for checking connection in loop()
static unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 30000; // 30s

void tickConfigLed() {
    int state = digitalRead(LED_BUILTIN);
    digitalWrite(LED_BUILTIN, !state);
}

// Callback: when ESP cant connect and switches to AP mode
void configModeCallback(WiFiManager *myWiFiManager) {
    WARN("Connection failed. Entering AP Mode for configuration.");
    INFO("Connect to AP: " + myWiFiManager->getConfigPortalSSID());

    wifiConfigTicker.attach(0.1, tickConfigLed);
}

void initWiFiManager() {
    WiFiManager wifiManager;

    // conf
    wifiManager.setDebugOutput(false); // ty, have my own logger
    wifiManager.setAPCallback(configModeCallback);

    // setting timeout for config portal
    // if noone connects, ESP tries to restart, or operate offline
    wifiManager.setTimeout(180000); // 180s

    LOG("Connecting to WiFi via WiFiManager...");

    // trying to connect
    if (!wifiManager.autoConnect("AlarmESP-setup")) {
        ERROR("Failed to connect and hit timeout");

        delay(3000); //3s
        // either offline or restart
    }

    // there is a connection if u here
    wifiConfigTicker.detach();
    digitalWrite(LED_BUILTIN, HIGH);
    LOG("WiFi connected succesfully!");

    // auto-reconnect
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true); // data stored in flash
} 

void handleWiFiConnection() {
    if(millis() - lastWiFiCheck >= WIFI_CHECK_INTERVAL) {
        lastWiFiCheck = millis();

        if(WiFi.status() != WL_CONNECTED) {
            WARN("WiFi connection lost. Reconnecting in backgournd...");
        }
    }
}

int getWiFiQuality() {
    if(WiFi.status() != WL_CONNECTED) return 0;
    long rssi = WiFi.RSSI();

    if(rssi <= -100) return 0;
    if(rssi >= -50) return 100;
    return 2 * (rssi + 100);
}