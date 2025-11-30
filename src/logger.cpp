#include "logger.h"
#include "sensorHandler.h"
#include "wifiHandler.h"

#include <ESP8266WiFi.h>
#include <time.h>

// Log server+client
WiFiServer logServer(7777);
WiFiClient logClient;

static int loopTicksThisMinute    = 0;
static unsigned long lastMinuteTs = 0;
static unsigned long startMillis  = millis();

static unsigned long getUptimeMinutes() {
  unsigned long now = millis();
  unsigned long elapsedMs = now - startMillis;
  return elapsedMs / 60000UL; // minutes
}

void loggerLoopTick() {
  loopTicksThisMinute++;
}

void loggerMinuteCheck() {
  unsigned long now = millis();
  if (now - lastMinuteTs < 60000UL) {
    return; // minute not passed
  }
  lastMinuteTs = now;

  int   sensorCalls   = 0;
  float avgDistanceCm = 0.0f;

  sensorStatsGet(sensorCalls, avgDistanceCm);

  unsigned long uptimeMinutes = getUptimeMinutes();
  int wifiQuality = getWiFiQuality();

  INFO("Minute stats -> uptime=" + String(uptimeMinutes) + 
       " min, loopTicks="        + String(loopTicksThisMinute) + 
       ", sensorCalls="          + String(sensorCalls) +
       ", avgDistance="          + String(avgDistanceCm, 1) + " cm" + 
       ", WiFi quality: "        + String(wifiQuality) + "%");

    loopTicksThisMinute = 0;
}

void logPrint(const String &msg) {
    Serial.print(msg);
    if(logClient && logClient.connected()) {
        logClient.print(msg);
  }
}

String getTimestamp() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
        timeinfo->tm_year + 1900, 
        timeinfo->tm_mon + 1, 
        timeinfo->tm_mday, 
        timeinfo->tm_hour, 
        timeinfo->tm_min, 
        timeinfo->tm_sec);
  return String(buf);
}

void initLogger() {
    // Start LOG server
    logServer.begin();
    logServer.setNoDelay(true);
    LOG("Log server started on port 7777");

}

void handleLogger() {
    if(!logClient || !logClient.connected()) {
    WiFiClient newClient = logServer.available();
    if(newClient) {
        logClient = newClient;
        RAW("\033[2J\033[H"); // ANSI escape
        INFO("AlarmESP-remake LOG console!\n");
        unsigned long uptimeMinutes = getUptimeMinutes();
        INFO("Current uptime: " + String(uptimeMinutes) + " min");
    }
  }
}

void shortBlink(int interval) {
  digitalWrite(LED_BUILTIN, LOW);   // ON
  delay(interval);
  digitalWrite(LED_BUILTIN, HIGH);  // OFF
  delay(interval);
  digitalWrite(LED_BUILTIN, LOW);   // ON
  delay(interval);
  digitalWrite(LED_BUILTIN, HIGH);  // OFF
}

