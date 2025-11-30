#pragma once
#include <Arduino.h>

void initWiFiManager();

void handleWiFiConnection(); 

int getWiFiQuality(); // RSSI signal (diagnostics)

bool isConfigResetRequested(); 

void clearConfigResetRequest();

void checkForConfigReset();
