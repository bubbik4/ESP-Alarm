#pragma once
#include <Arduino.h>

void initWiFiManager();

void checkForConfigReset();
bool isConfigResetRequested(); 
void clearConfigResetRequest();
