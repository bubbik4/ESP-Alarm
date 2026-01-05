#pragma once
#include <Arduino.h>

enum SystemState {
    STATE_BOOT,
    STATE_DISARMED,
    STATE_ARMED,
    STATE_ALARM,
    STATE_WIFI_LOST
};

void initLeds();
void handleLeds();
void setLedState(SystemState newState);