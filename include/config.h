#pragma once
#include <Arduino.h>

#define DEBUG_MODE 1
#if DEBUG_MODE
    #define LOG(x) Serial.println(x)
    #define LOGF(...) Serial.printf(__VA_ARGS__)
#else
    #define LOG(x)
    #define LOGF(...)
#endif
