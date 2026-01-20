#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>

#include "wifiHandler.h"
#include "logger.h"
#include "sensorHandler.h"
#include "otaHandler.h"
#include "mqttHandler.h"
#include "ledHandler.h"

static uint32_t lastCheckTime = 0;
const unsigned long SENSOR_INTERVAL = 500;

void setup() {
  Serial.begin(115200);
  delay(500);
  RAW("=== ESP ALARM on MQTT ===");

  digitalWrite(LED_BUILTIN, 0);

  Serial.begin(115200);

  initLeds();
  initSensor();
  

  initWiFiManager();
  WiFi.setHostname("AlarmESP-remake");


  initOTA();


  initMQTT();

  digitalWrite(LED_BUILTIN, 1);
  setLedState(STATE_ARMED);
}

void loop() {

  handleOTA();
  handleLeds();
  handleMQTT();

  checkResetButton();
  checkForConfigReset();

  // Sensor calling logic
  if(millis() - lastCheckTime >= SENSOR_INTERVAL) {
    lastCheckTime = millis();
    handleSensor();
  }
}

