#include "ledHandler.h"
#include <Adafruit_NeoPixel.h>

#define PIN_LEDS D2
#define NUM_LEDS 6

Adafruit_NeoPixel strip(NUM_LEDS, PIN_LEDS, NEO_GRB + NEO_KHZ800);

SystemState currentState = STATE_BOOT;
unsigned long lastUpdate = 0;
unsigned long stateChangeTime = 0; // Tu zapisujemy, kiedy zmienił się stan

int brightness = 0;
int fadeAmount = 5;
bool blinkState = false;

// Czas wygaszania w stanie DISARMED (ms)
const unsigned long LED_TIMEOUT = 60000; // 60 sekund

void initLeds() {
    strip.begin();
    strip.setBrightness(100);
    strip.show();
}

uint32_t getColor(uint8_t r, uint8_t g, uint8_t b) {
    return strip.Color(r, g, b);
}

void reportError(int count, uint32_t color) {
    int oldBrightness = strip.getBrightness();
    strip.setBrightness(150);

    for(int i = 0; i < 3; i++) {
        strip.fill(strip.Color(255, 255, 255)); // BIAŁY
        strip.show();
        delay(100);
        strip.clear();
        strip.show();
        delay(100);
    }

    delay(500);

    //err code
    for(int i = 0; i < count; i++) {
        strip.fill(color);
        strip.show();
        delay(400); // Dłuższe świecenie
        strip.clear();
        strip.show();
        delay(300); // Przerwa
    }


    strip.setBrightness(oldBrightness);
}

void colorWipe(uint32_t color) {
    for(int i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

void handleLeds() {
    unsigned long now = millis();

    switch (currentState) {
        case STATE_BOOT:
            if (now - lastUpdate > 100) {
                lastUpdate = now;
                static int i = 0;
                strip.clear();
                strip.setPixelColor(i, strip.Color(0, 0, 255));
                strip.show();
                i = (i + 1) % NUM_LEDS;
            }
            break;

        case STATE_DISARMED:
            // Sprawdź czy minęła minuta od rozbrojenia
            if (now - stateChangeTime > LED_TIMEOUT) {
                // Minął czas - wygaś wszystko
                strip.clear();
                strip.show();
            } else {
                // Czas nie minął - świeć na zielono (oddychanie lub stały)
                // Tu prosta wersja: stały zielony, odświeżany co sekundę
                if (now - lastUpdate > 1000) {
                    lastUpdate = now;
                    colorWipe(strip.Color(0, 255, 0));
                }
            }
            break;

        case STATE_ARMED:
            // Czerwone pulsowanie
            if (now - lastUpdate > 30) {
                lastUpdate = now;
                brightness += fadeAmount;
                if (brightness <= 10 || brightness >= 150) {
                    fadeAmount = -fadeAmount;
                }
                // Efekt pulsowania tylko kanałem R
                strip.fill(strip.Color(brightness, 0, brightness/2));
                strip.show();
            }
            break;

        case STATE_ALARM:
            if (now - lastUpdate > 100) {
                lastUpdate = now;
                blinkState = !blinkState;
                if(blinkState) {
                    colorWipe(strip.Color(255, 0, 0));
                } else {
                    colorWipe(strip.Color(0, 0, 255));
                }
            }
            break;
            
        case STATE_WIFI_LOST:
             if (now - lastUpdate > 500) {
                lastUpdate = now;
                blinkState = !blinkState;
                colorWipe(blinkState ? strip.Color(255, 200, 0) : 0);
             }
             break;
    }
}

void setLedState(SystemState newState) {
    currentState = newState;
    stateChangeTime = millis(); // RESETUJEMY timer przy każdej zmianie stanu!
    
    // Reset zmiennych animacji
    brightness = 50; 
    strip.clear();
    strip.show();
}

void updateOtaProgress(int percent) {
    int ledsToLight = map(percent, 0, 100, 0, NUM_LEDS);

    uint32_t color = strip.Color(0,255,255);

    if(percent >=100) {
        color = strip.Color(0,255,0);
    }

    strip.clear();
    for(int i = 0; i < ledsToLight; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}