#pragma once
#include <Arduino.h>

void initMQTT();
void handleMQTT();
void sendMQTTStatus();
void sendMQTTAlarm(float distance);