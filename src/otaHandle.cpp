#include <Arduino.h>
#include <ArduinoOTA.h>

#include "logger.h"
#include "otaHandler.h"
#include "ledHandler.h"

void initOTA() {
  ArduinoOTA.setHostname("AlarmESP-remake");

  ArduinoOTA.onStart([]() {
    LOG("OTA Start");
    updateOtaProgress(0);
  });
  ArduinoOTA.onEnd([]() {
    LOG("OTA End");
    updateOtaProgress(100);
    WARN("Connection closed, please restart PuTTY");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    ERROR(String("OTA Error[%u]\n") + String(error));
  });
ArduinoOTA.onProgress([](unsigned long progress, unsigned int total) {
    // Oblicz procent
    int percent = progress / (total / 100);
    
    // Zmienna statyczna pamięta wartość między wywołaniami funkcji
    static int lastLeds = -1;
    
    // Mapujemy 0-100% na liczbę diod (np. 0-6)
    // Zakładamy, że NUM_LEDS jest dostępne (lub wpisz 6 na sztywno)
    int ledsToLight = map(percent, 0, 100, 0, 6); 

    // Aktualizuj pasek TYLKO JEŚLI zmieniła się liczba diod do zapalenia
    if (ledsToLight != lastLeds) {
        lastLeds = ledsToLight;
        updateOtaProgress(percent);
    }

    // Logowanie do konsoli co 10% (zostawiamy dla diagnostyki)
    static int lastLogPercent = -1;
    if (percent % 10 == 0 && percent != lastLogPercent) {
        lastLogPercent = percent;
        // Używamy Serial.println zamiast INFO/LOG żeby nie zapychać sieci w trakcie OTA
        Serial.printf("[OTA] Progress: %d%%\n", percent); 
    }
  });

  ArduinoOTA.begin();
  INFO("OTA Ready");
}

void handleOTA() {
    ArduinoOTA.handle();
}