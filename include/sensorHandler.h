#pragma once
#include <Arduino.h>

void initSensor();
void handleSensor();

bool isAlarmTriggered();
float getLastDistance();
void resetAlarm();

extern int alarmArmed;