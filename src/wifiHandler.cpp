#include "wifiHandler.h"
#include "logger.h"
#include "sensorHandler.h"
#include "ledHandler.h"

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <Ticker.h>

Ticker wifiConfigTicker;
WiFiManager wifiManager;

// Timer for checking connection in loop()
// static unsigned long lastWiFiCheck = 0;
// const unsigned long WIFI_CHECK_INTERVAL = 30000; // 30s

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
    // conf
    wifiManager.setDebugOutput(false); // ty, have my own logger
    wifiManager.setAPCallback(configModeCallback);

    // setting timeout for config portal
    // if noone connects, ESP tries to restart, or operate offline
    wifiManager.setTimeout(180); // 180s

    LOG("Connecting to WiFi via WiFiManager...");

    // trying to connect
    if (!wifiManager.autoConnect("AlarmESP-setup")) {
        ERROR("Failed to connect and hit timeout");

        delay(3000); //3s
        // either offline or restart
        ESP.restart();
    }

    // there is a connection if u here
    wifiConfigTicker.detach();
    digitalWrite(LED_BUILTIN, HIGH);

    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    // auto-reconnect
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true); // data stored in flash

    LOG("WiFi connected succesfully! IP: " + WiFi.localIP().toString());
}

void checkForConfigReset() {
    if (isConfigResetRequested()) {
        ALERT("ERASING WIFI SETTINGS...");
        reportError(4, getColor(200, 0, 255));

        wifiManager.resetSettings();
        clearConfigResetRequest();

        delay(1000);
        ESP.restart();
    }
}