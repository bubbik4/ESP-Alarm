#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>

#include "secrets.h"
#include "config.h"

class OtaManager {
private:
    const char* _hostname;
    const char* _password;

public:
    OtaManager(const char* hostname, const char* password);

    void setup();
    void handle();
};