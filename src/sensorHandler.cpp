#include "sensorHandler.h"
#include "logger.h"
#include "telegramBotID.h"
#include "telegramHandler.h"
#include <Ticker.h>

const int trig = D6;
const int echo = D5;
const int buzz = D8;

volatile bool buzzerState = false;
Ticker buzzerTicker;

static bool alarmTriggered = false;
int alarmArmed = 1;

static float distance = 0;
static float duration = 0;

static int failCount = 0;

static float readings[5] = {0};
static int readIndex = 0;
static float sum = 0;

static unsigned long lastAlarmMessage = 0;
const int ALARM_COOLDOWN = 30000;

const float OPEN_THRESHOLD = 15.0; // cm
const float CLOSE_THRESHOLD = 8.0; // cm

static int   sensorCallsThisMinute = 0;
static float distanceSumThisMinute = 0.0f;

void buzzerISR() {
  if (alarmTriggered) {
    buzzerState = !buzzerState;
    digitalWrite(buzz, buzzerState);        // flip
  } else {
    buzzerState = false;
    digitalWrite(buzz, LOW);                // off
  }
}

float getFilteredDistance() {
  duration   = pulseIn(echo, HIGH, 15000);
  float dist = (duration * 0.0343f) / 2.0f;

  sum -= readings[readIndex];
  readings[readIndex] = dist;
  sum += readings[readIndex];

  readIndex = ((readIndex+1) % 5);

  return sum / 5.0f; // last 5 reads avg
}


void sendAlarmNotification(float dist) {
  if(millis() - lastAlarmMessage > ALARM_COOLDOWN) {
    lastAlarmMessage = millis();

    String message = "AlarmESP-remake\nalarm triggered!\n";
    message       += "Measured distance: " + String(dist, 1) + " cm.\n";

    queueTelegramMessage(String(ALLOWED_CHAT_ID), message);
  } else {
    unsigned long elapsed = millis() - lastAlarmMessage;
    long remaining = ALARM_COOLDOWN - (long)elapsed;
    if(remaining > 0) {
      WARN("Telegram message not sent. Message cooldown not met. (" + String(remaining / 1000.0, 1) + " s)");
    } else {
      remaining = 0;
      ERROR("Telegram message not sent. Door open for too long to reset the cooldown.");
    }
  } 
}

bool handleAlarm(float dist) {
  if(dist > OPEN_THRESHOLD) {                  // door open
   if(!alarmTriggered && alarmArmed) {
      alarmTriggered = true;
      LOG("Alarm triggered at distance: " + String(dist, 1) + " cm");
      sendAlarmNotification(dist);
      return 1;
   }  return 0;
  } else if(dist < CLOSE_THRESHOLD) {          // door closed
    if(alarmTriggered) {
      LOG("Door closed. Alarm reset at distance: " + String(dist, 1) + " cm");
      alarmTriggered = false;
    }
    return 0;
  }

  // deadzone
  return 0;
}

void initSensor() {
    pinMode(trig, OUTPUT);
    pinMode(echo, INPUT);
    pinMode(buzz, OUTPUT);
    digitalWrite(buzz, LOW);

    buzzerTicker.attach_ms(100, buzzerISR);
}

void handleSensor() {


  digitalWrite(trig, 0);
  delayMicroseconds(2);
  digitalWrite(trig, 1);
  delayMicroseconds(10);
  digitalWrite(trig, 0);

  distance = getFilteredDistance();
  // distance = getMedianDistance();
  if(duration <= 0) {
    failCount++;
    WARN("Sensor read failure [" + String(failCount) + "]");
    if(failCount >= 10) {
      ERROR("No answer from sensor in 10 tries. Rebooting"); 
      delay(1000);
      ESP.restart();
    }
  } else {
      // This means the sensor read is correct
      failCount = 0;
      handleAlarm(distance);
      sensorStatsTick(distance);
  }
}

bool isAlarmTriggered() {
    return alarmTriggered;
}

float getLastDistance() {
    return distance;
}

void resetAlarm() {
    alarmTriggered = false;
    buzzerState = false;
    digitalWrite(buzz, LOW);
}

void sensorStatsTick(float lastDistance) {
  sensorCallsThisMinute++;
  distanceSumThisMinute += lastDistance;
}

void sensorStatsGet(int &calls, float &avgDistance) {
  calls = sensorCallsThisMinute;
  if (sensorCallsThisMinute > 0) {
    avgDistance = distanceSumThisMinute / sensorCallsThisMinute;
  } else {
    avgDistance = 0.0f;
  }

  sensorCallsThisMinute = 0;
  distanceSumThisMinute = 0.0f;
}