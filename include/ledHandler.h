#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

enum SystemState {
    STATE_BOOT,
    STATE_DISARMED,
    STATE_ARMED,
    STATE_ALARM,
    STATE_WIFI_LOST,
    STATE_ERROR
};

void initLeds();
void handleLeds();
void setLedState(SystemState newState);

void updateOtaProgress(int percent);

void reportError(int count, uint32_t color);

uint32_t getColor(uint8_t r, uint8_t g, uint8_t b);