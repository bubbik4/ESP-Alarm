#pragma once
#include <Arduino.h>

void initSensor();
void handleSensor();

void checkResetButton();

bool isAlarmTriggered();
float getLastDistance();
void resetAlarm();

extern int alarmArmed;
