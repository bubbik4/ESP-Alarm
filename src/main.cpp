#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>

#include "wifiAuth.h"

#include "logger.h"
#include "sensorHandler.h"
#include "otaHandler.h"
#include "telegramHandler.h"

const char*  ntpServer          = "pool.ntp.org";
const long   gmtOffset_sec      = 3600; // GMT+!
const int    daylightOffset_sec = 0;


static uint32_t lastCheckTime = 0;
const unsigned long SENSOR_INTERVAL = 500;




void setupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  INFO("Waiting for NTP time sync...");
  int retry = 0;
  time_t now = time(nullptr);
  while (now < 1000000000 && retry <30) {
    //waiting for correct time
    delay(500);
    INFO("still waiting...");
    now = time(nullptr);
    retry++;
  }
  if(now < 1000000000) {
    WARN("Failed to get time from NTP, logs won't have accurate timestamps");
  } else {
    LOG("NTP time synchronized");
  }
}



void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  Serial.begin(115200);
  delay(1000);
  RAW("=== ESP ALARM REMAKE ===");

  // WiFi setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  LOG("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    LOG(".");
  }
  LOG("WiFi connected. IP: " + WiFi.localIP().toString());
  WiFi.setHostname("AlarmESP-remake");

  initLogger();

  setupTime();

  initOTA();

  initSensor();

  initTelegram();

  digitalWrite(LED_BUILTIN, 1);
  shortBlink();
}

void loop() {
  loggerLoopTick();
  handleOTA();

  handleLogger();

  // Sensor calling logic
  if(millis() - lastCheckTime >= SENSOR_INTERVAL) {
    lastCheckTime = millis();
    if(alarmArmed) {
      handleSensor();
    }
  }

   // Telegram | sending + recieveing
  handleTelegramSending();
  handleTelegramUpdates();

  loggerMinuteCheck();

  // Time log
// static uint32_t lastUptimeLog = 0;
// if (millis() - lastUptimeLog >= 60000) {
//   lastUptimeLog += 60000;    // albo lastUptimeLog = millis();

//   uptimeMinutes = millis() / 60000;
//   LOG(String("Uptime: ") + uptimeMinutes + " min");
//   shortBlink(50);
//   INFO("Loop ticks this minute: " + String(loopTicksThisMinute));  // DEBUG
//   loopTicksThisMinute = 0;                                         // DEBUG
//   if (samples > 0) {
//     INFO("Sensor last minute average read: " 
//       + String(distanceLastMinute / samples)
//       + " cm (samples: " + String(samples)
//       + ", handleSensor calls: " + String(sensorCallsThisMinute)
//       + ")");
//   } else if (!alarmArmed) {
//     INFO("Distance measuring OFF.");
//   } else {
//     WARN("This shouldn't happen. If you see this, something is not working as it should.");
//   }

// }
}

