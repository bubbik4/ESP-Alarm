#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoOTA.h>
#include <UniversalTelegramBot.h>
#include <WiFiUdp.h>
#include <time.h>
#include <Ticker.h>

#include "wifiAuth.h"
#include "telegramBotID.h"

const char*  ntpServer = "pool.ntp.org";
const long   gmtOffset_sec = 3600; // GMT+!
const int    daylightOffset_sec = 0;


WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

unsigned long lastAlarmMessage = 0;
const int ALARM_COOLDOWN = 30000;

WiFiServer logServer(7777); // Log port
WiFiClient logClient;

// ANSI color codes
#define COL_RESET "\033[0m"
#define COL_WHITE   "\033[37m" 
#define COL_BLUE    "\033[94m"
#define COL_YELLOW  "\033[33m"
#define COL_RED     "\033[31m"
#define COL_BRED    "\033[1;31m"  // bold red
String getTimestamp();

// LOG macro
#define RAW(msg)    logPrint(getTimestamp() + " " + String(msg) + "\r\n")
#define ALERT(msg)  logPrint(getTimestamp() + " " +   String(COL_BRED) + "  [ALERT] " + String(msg) + String(COL_RESET) + "\r\n")
#define LOG(msg)    logPrint(getTimestamp() + " " +  String(COL_WHITE) + "  [LOG]   " + String(msg) + String(COL_RESET) + "\r\n")
#define INFO(msg)   logPrint(getTimestamp() + " " +   String(COL_BLUE) + "  [INFO]  " + String(msg) + String(COL_RESET) + "\r\n")
#define WARN(msg)   logPrint(getTimestamp() + " " + String(COL_YELLOW) + "  [WARN]  " + String(msg) + String(COL_RESET) + "\r\n")
#define ERROR(msg)  logPrint(getTimestamp() + " " +    String(COL_RED) + "  [ERROR] " + String(msg) + String(COL_RESET) + "\r\n")


void logPrint(const String &msg) {
  Serial.print(msg);
  if(logClient && logClient.connected()) {
    logClient.print(msg);
  }
}

Ticker buzzerTicker;
void buzzerISR() {
  if (alarmTriggered) {
    digitalWrite(buzz, !digitalRead(buzz)); // flip
  } else {
    digitalWrite(buzz, LOW);                // off
  }
}

const int trig = D6;
const int echo = D5;

const int buzz = D8;

int failCount = 0;

int alarmArmed = 1;
bool alarmTriggered = false;
float distance;
float duration;
int samples = 0;
int uptimeMinutes;
float distanceLastMinute = 0;

bool pendingTelegram = false;
String pendingMessage;
unsigned long telegramLastAttempt = 0;
const unsigned long TELEGRAM_RETRY_INTERVAL = 1000; // 1s between tries
uint8_t telegramRetryCount = 0;
const uint8_t TELEGRAM_MAX_RETRIES = 3;

unsigned long lastBotCheck = 0;
const unsigned long BOT_CHECK_INTERVAL = 1000; // 1s

float readings[5];
int readIndex = 0;
float sum = 0;

static uint32_t lastCheckTime = 0;

void handleNewMessages(int numNewMessages) {
  for(int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from = bot.messages[i].from_name;

    INFO("Telegram message from: " + from + " (" + chat_id + "): " + text);

    //normalize
    text.trim();

    if(text == "/alarm") {
      //switch state
      alarmArmed = !alarmArmed;

      String state = alarmArmed ? "UZBROJONY" : "ROZBROJONY";
      String reply = "AlarmESP-remake\nStan alarmu został zmieniony na : *" + state + "*.";
      if(alarmArmed) {
        INFO("Armed via Telegram.");
      } else {
        INFO("Disarmed via Telegram.");
      }

      if(!alarmArmed && alarmTriggered) {
        alarmTriggered = false;
        digitalWrite(buzz, LOW);
        INFO("Alarm disarmed via Telegram. Buzzer turned off.");
      }

      bool ok = bot.sendMessage(chat_id, reply, "Markdown");
      if(ok) {
        INFO("Telegram reply sent (alarm state: (" + state + ")");
      } else {
        ERROR("Failed to send Telegram reply for /alarm command.");
      }

    } else if( text == "/status") {
      String state = alarmArmed ? "UZBROJONY" : "ROZBROJONY";
      String msg = "AlarmESP-remake status report:\n"
                   "• Alarm state: *" + state + "*\n"
                   "• Last read: " + String(distance, 1) + " cm";

      bot.sendMessage(chat_id, msg, "Markdown");

    } else if (text == "/start" || text == "/help") {
      String msg = "Help menu:\n"
                   "Available commands:\n"
                   "/alarm – switch alarm arming\n"
                   "/status – show current device state";
      bot.sendMessage(chat_id, msg, "Markdown");

    } else {
      String msg = "Unknown command.\nUse /alarm or /status.";
      bot.sendMessage(chat_id, msg, "");
    }
  }
}


void setupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  INFO("Waiting for NTP time sync...");
  int retry = 0;
  time_t now = time(nullptr);
  while (now < 1000000000 && retry <10) {
    //waiting for correct time
    delay(500);
    INFO("still waiting...");
    now = time(nullptr);
    retry++;
  }
  if(now < 1000000000) {
    WARN("Failed to get time from NTP, logs won't have accurate timestamps");
  } else {
    INFO("NTP time synchronized");
  }
}

String getTimestamp() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
  timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  return String(buf);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  Serial.begin(115200);
  delay(999);
  RAW("=== ESP ALARM REMAKE ===");

  // WiFi setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  LOG("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    LOG(".");
  }
  INFO("WiFi connected. IP: " + WiFi.localIP().toString());
  WiFi.setHostname("AlarmESP-remake");

  // Start LOG server
  logServer.begin();
  logServer.setNoDelay(true);
  LOG("Log server started on port 7777");

  uint32_t startWait = millis();
  while (millis() - startWait < 5000) {
    ArduinoOTA.handle();
    WiFiClient newClient = logServer.available();
    if(newClient) {
      logClient = newClient;
      logClient.print("\033c"); // clearing PuTTY
      INFO("Client connected, console cleared");
      break;
    }
    delay(10);
  }
  setupTime();

  pendingTelegram = false;
  pendingMessage = "";
  telegramRetryCount = 0;

  secured_client.setInsecure();
  LOG("Telegram client configured (insecure mode)");

 
  // OTA
  ArduinoOTA.setHostname("AlarmESP-remake");

  ArduinoOTA.onStart([]() {
    INFO("OTA Start");
    delay(500);
  });
  ArduinoOTA.onEnd([]() {
    INFO("OTA End");
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

  INFO("IP address: " + WiFi.localIP().toString());

  
  // Sensor
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  pinMode(buzz, OUTPUT);
  digitalWrite(buzz, LOW);

  digitalWrite(LED_BUILTIN, 1);

  // Every 200ms changing buzzer state 
  buzzerTicker.attach_ms(200, buzzerISR); 
}

float getFilteredDistance() {
  duration = pulseIn(echo, HIGH, 30000);
  float dist = (duration * 0.0343) / 2.0;

  sum -= readings[readIndex];
  readings[readIndex] = dist;
  sum += readings[readIndex];

  readIndex = ((readIndex+1) % 5);

  return sum / 5.0; // last 5 reads avg
}

void sendAlarmNotification(float dist) {
  if(millis() - lastAlarmMessage > ALARM_COOLDOWN) {
    lastAlarmMessage = millis();

    String message = "AlarmESP-remake\nwykrył otwarcie drzwi!\n";
    message+= ("Zmierzona odległość: " + String(dist, 1) + " cm.");

    pendingMessage = message;
    pendingTelegram = true;
    LOG("Queued Telegram message for sending");

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
  if(dist > 10) {                 // door open
   if(!alarmTriggered) {
      alarmTriggered = true;
      INFO("Alarm triggered.");
      sendAlarmNotification(dist);
      return 1;
   } return 0;
  } else {                       // door closed
    if(alarmTriggered) {
      INFO("Door closed. Alarm reset.");
      alarmTriggered = false;
    }
    return 0;
  }
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
      // LOG("[LOG] Sensor read: " + String(distance) + " cm"); 
      distanceLastMinute+=distance;
      samples++;
      handleAlarm(distance);
  }
}

void handleTelegramSending() {
  if(!pendingTelegram) return;
  if (millis() - telegramLastAttempt < TELEGRAM_RETRY_INTERVAL) return;

  telegramLastAttempt = millis();

  if(telegramRetryCount >= TELEGRAM_MAX_RETRIES) {
    ERROR("Giving up on Telegram message after max retries.");
    pendingMessage = "";
    pendingTelegram = false;
    telegramRetryCount = 0;
    return;
  }

  ALERT("Sending Telegram message...");
  bool ok = bot.sendMessage(CHAT_ID, pendingMessage, "Markdown");
  if(ok) {
    INFO("Telegram message sent succesfully.");
    pendingTelegram = false;
    pendingMessage = "";
    telegramRetryCount = 0;
  } else {
    telegramRetryCount++;
    ERROR("Failed to send Telegram message (sendMessage() == false). Retry " + String(telegramRetryCount) + "/" + String(TELEGRAM_MAX_RETRIES));
    //pendingTelegram stays true, next try in one sec.
  }
}

void loop() {
  ArduinoOTA.handle();
  
  // Log handle
  if(!logClient || !logClient.connected()) {
    WiFiClient newClient = logServer.available();
    if(newClient) {
      logClient = newClient;
      RAW("\033[2J\033[H"); // ANSI escape
      INFO("AlarmESP-remake LOG console!\n");
      INFO("Current uptime: " + String(uptimeMinutes) + " min");
      INFO("Current alarm state: " + alarmArmed ? "ARMED" : "DISARMED");
    }
  }

  // Time log
  static uint32_t last = 0;
  if (millis() - last > 1000) {
    last = millis();
    uint32_t uptime = millis() / 1000;
    if(!(uptime % 60)) {
      uptimeMinutes = uptime / 60;
      LOG(String("Uptime: ") + uptimeMinutes + " min");

      //additional avg sensor log
      if(samples > 0) {
        INFO("Sensor last minute average read: " + String(distanceLastMinute/(samples)) + " cm (samples: " + String(samples) + ")");
      } else if(!alarmArmed) {
        INFO("Distance measuring OFFD.");
      } else {
        WARN("This shouldn't happen. If you see this, something is not working as it should.");
      }
      distanceLastMinute = 0;
      samples = 0;
    }
}


  // Main guy here
  if(millis() - lastCheckTime >= 500) {
    lastCheckTime = millis();
    if(alarmArmed) {
      handleSensor();
    }
  }

   // Here was func for toggling buzzer. discontinued.

  handleTelegramSending();

  if (millis() - lastBotCheck > BOT_CHECK_INTERVAL) {
    lastBotCheck = millis();
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}




