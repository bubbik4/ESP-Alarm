#include "otaHandler.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include "logger.h"

void initOTA() {
  ArduinoOTA.setHostname("AlarmESP-remake");

  ArduinoOTA.onStart([]() {
    LOG("OTA Start");
    delay(500);
  });
  ArduinoOTA.onEnd([]() {
    LOG("OTA End");
    WARN("Connection closed, please restart PuTTY");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    ERROR(String("OTA Error[%u]\n") + String(error));
  });
  ArduinoOTA.onProgress([](unsigned long progress, unsigned int total) {
    int rprg = progress / (total/100);
    if(!(rprg%10)) {
      INFO("[OTA] Progress: " + String(rprg) + "%");
    }
  });

  ArduinoOTA.begin();
  INFO("OTA Ready");
}

void handleOTA() {
    ArduinoOTA.handle();
}