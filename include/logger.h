#pragma once
#include <Arduino.h>
inline void initLogger();
inline void handleLogger();
inline void loggerLoopTick();
inline void loggerMinuteCheck();


#define INFO(x)  Serial.println(String("[INFO]  ") + x)
#define WARN(x)  Serial.println(String("[WARN]  ") + x)
#define ERROR(x) Serial.println(String("[ERROR] ") + x)
#define LOG(x)   Serial.println(String("[LOG]   ") + x)
#define ALERT(x) Serial.println(String("[ALERT] ") + x)
#define RAW(x)   Serial.println(x)