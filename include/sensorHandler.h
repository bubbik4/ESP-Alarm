#pragma once
#include <Arduino.h>

void initSensor();
void handleSensor();

bool isAlarmTriggered();
float getLastDistance();
void resetAlarm();
void sensorStatsTick(float lastDistance); 
void sensorStatsGet(int &calls, float &avgDistance);

extern int alarmArmed;
