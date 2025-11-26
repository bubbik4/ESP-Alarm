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

const char*  ntpServer          = "pool.ntp.org";
const long   gmtOffset_sec      = 3600; // GMT+!
const int    daylightOffset_sec = 0;


WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

unsigned long lastAlarmMessage = 0;
const int ALARM_COOLDOWN       = 30000;

WiFiServer logServer(7777); // Log port
WiFiClient logClient;

// ANSI color codes
#define COL_RESET   "\033[0m"
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

bool alarmTriggered = false;
const unsigned long SENSOR_INTERVAL = 500;

const int trig = D6;
const int echo = D5;

const int buzz = D8;
volatile bool buzzerState = false;
Ticker buzzerTicker;
void buzzerISR() {
  if (alarmTriggered) {
    buzzerState = !buzzerState;
    digitalWrite(buzz, buzzerState);        // flip
  } else {
    buzzerState = false;
    digitalWrite(buzz, LOW);                // off
  }
}

int failCount = 0;

int alarmArmed = 1;
float distance;
float duration;
int samples = 0;
int uptimeMinutes;
float distanceLastMinute = 0;

struct TgMessage {
  String chatId;
  String text;
};

bool pendingTelegram = false;
TgMessage pendingMsg;
unsigned long telegramLastAttempt           = 0;
const unsigned long TELEGRAM_RETRY_INTERVAL = 1000; // 1s between tries
uint8_t telegramRetryCount                  = 0;
const uint8_t TELEGRAM_MAX_RETRIES          = 3;

uint32_t msgCounter                    = 0;
uint32_t lastSentMsgId                 = 0;
unsigned long lastSentMsgTime          = 0;
const unsigned long DUPLICATE_WINDOW   = 5000;          // 5s

unsigned long lastBotCheck             = 0;
const unsigned long BOT_CHECK_INTERVAL = 5000;      // 5s

float readings[5];
int readIndex = 0;
float sum     = 0;

int loopTicksThisMinute = 0; // DEBUG

static uint32_t lastCheckTime = 0;

void shortBlink(int interval = 100) {
  digitalWrite(LED_BUILTIN, LOW);   // ON
  delay(interval);
  digitalWrite(LED_BUILTIN, HIGH);  // OFF
  delay(interval);
  digitalWrite(LED_BUILTIN, LOW);   // ON
  delay(interval);
  digitalWrite(LED_BUILTIN, HIGH);  // OFF
}

void queueTelegramMessage(const String &chatId, const String &msg, bool force = false) {
  // if something is waiting, not forcing, dont overwrite
  if(pendingTelegram && !force) {
    WARN("Telegram message already pending, new one dropped.");
    return;
  }

  msgCounter++;
  pendingMsg.chatId   = chatId;
  pendingMsg.text     = msg + "\nEvent ID: #" + String(msgCounter);
  pendingTelegram     = true;
  telegramRetryCount  = 0;
  telegramLastAttempt = 0; // allows to send msg in next handleTelegramSending()

  INFO("Queued Telegram message for sending (id #" + String(msgCounter) + ")");
}

void handleNewMessages(int numNewMessages) {
  for(int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from = bot.messages[i].from_name;

    LOG("Telegram message from: " + from + " (" + chat_id + "): " + text);
    shortBlink();

    String reply;

    if(chat_id.equals(ALLOWED_CHAT_ID)) {
      text.trim();

      if(text == "/alarm") {
        //switch state
        alarmArmed = !alarmArmed;

        String state = alarmArmed ? "ARMED" : "DISARMED";
        reply        = "AlarmESP-remake\nAlarm state switched to: *" + state + "*.";
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

      } else if( text == "/status") {
        String state = alarmArmed ? "UZBROJONY" : "ROZBROJONY";
        reply =   "AlarmESP-remake status report:\n";
        reply +=  "• Alarm state: *" + state + "*\n";
        reply +=  "• Last read: " + String(distance, 1) + " cm";

      } else if (text == "/start" || text == "/help") {
        reply = "Help menu:\n"
                "Available commands:\n"
                "/alarm – switch alarm arming\n"
                "/status – show current device state";

      } else {
        reply = "Unknown command.\nUse /alarm or /status.";
      }
    } else {
      WARN("unknown_exception found: user not whitelisted. (" + String(chat_id) + ")");
    }

    if(reply.length() > 0) {
      queueTelegramMessage(chat_id, reply);
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
    LOG("NTP time synchronized");
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
  LOG("WiFi connected. IP: " + WiFi.localIP().toString());
  WiFi.setHostname("AlarmESP-remake");

  // Start LOG server
  logServer.begin();
  logServer.setNoDelay(true);
  LOG("Log server started on port 7777");

  uint32_t startWait = millis();
  while (millis() - startWait < 3000) {
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

  secured_client.setInsecure();
  LOG("Telegram client configured (insecure mode)");

 
  // OTA
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

  LOG("IP address: " + WiFi.localIP().toString());

  
  // Sensor
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  pinMode(buzz, OUTPUT);
  digitalWrite(buzz, LOW);

  digitalWrite(LED_BUILTIN, 1);

  // Every 200ms changing buzzer state 
  buzzerTicker.attach_ms(100, buzzerISR); 
  shortBlink();
}

float getFilteredDistance() {
  duration   = pulseIn(echo, HIGH, 15000);
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



    String message = "AlarmESP-remake\nalarm triggered!\n";
    message       += "Measured distance: " + String(dist, 1) + " cm.\n";

    queueTelegramMessage(ALLOWED_CHAT_ID, message);
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
  const float OPEN_THRESHOLD = 15.0;           // cm 
  const float CLOSE_THRESHOLD = 8.0;           // cm

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

int sensorCallsThisMinute = 0;

void handleSensor() {

  sensorCallsThisMinute++;

  digitalWrite(trig, 0);
  delayMicroseconds(2);
  digitalWrite(trig, 1);
  delayMicroseconds(10);
  digitalWrite(trig, 0);

  distance = getFilteredDistance();
  // distance = getMedianDistance();
  if(duration <= 0) {
    failCount++;
    shortBlink(20);
    WARN("Sensor read failure [" + String(failCount) + "]");
    if(failCount >= 10) {
      ERROR("No answer from sensor in 10 tries. Rebooting"); 
      delay(1000);
      ESP.restart();
    }
  } else {
      // This means the sensor read is correct
      failCount           = 0;
      distanceLastMinute += distance;
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
    pendingMsg.text     = "";
    pendingMsg.chatId   = "";
    pendingTelegram     = false;
    telegramRetryCount  = 0;
    return;
  }

  ALERT("Sending Telegram message to chatId=" + pendingMsg.chatId + 
        " (len=" + String(pendingMsg.text.length()) + ")");
  bool ok = bot.sendMessage(pendingMsg.chatId, pendingMsg.text, "Markdown");

  if(ok) {
    INFO("Telegram message sent succesfully.");
    pendingTelegram    = false;
    lastSentMsgId      = msgCounter;
    lastSentMsgTime    = millis();
    pendingMsg.text    = "";
    pendingMsg.chatId  = "";
    telegramRetryCount = 0;
    shortBlink(20);

  } else {
    telegramRetryCount++;
    ERROR("Failed to send Telegram message (sendMessage() == false). Retry " + String(telegramRetryCount) + "/" + String(TELEGRAM_MAX_RETRIES));
    ERROR("Telegram lastError: " + String(bot._lastError));
    //pendingTelegram stays true, next try in one sec.
  }
}

void loop() {
  loopTicksThisMinute++;  // DEBUG
  ArduinoOTA.handle();
  
  // Log handle
  if(!logClient || !logClient.connected()) {
    WiFiClient newClient = logServer.available();
    if(newClient) {
      shortBlink();

      logClient = newClient;
      RAW("\033[2J\033[H"); // ANSI escape
      INFO("AlarmESP-remake LOG console!\n");
      INFO("Current uptime: " + String(uptimeMinutes) + " min");
      String stateEng = alarmArmed ? "ARMED" : "DISARMED";
      INFO("Current alarm state: " + stateEng);
    }
  }

  // Time log
static uint32_t lastUptimeLog = 0;
if (millis() - lastUptimeLog >= 60000) {
  lastUptimeLog += 60000;    // albo lastUptimeLog = millis();

  uptimeMinutes = millis() / 60000;
  LOG(String("Uptime: ") + uptimeMinutes + " min");
  shortBlink(50);
  INFO("Loop ticks this minute: " + String(loopTicksThisMinute));  // DEBUG
  loopTicksThisMinute = 0;                                         // DEBUG
  if (samples > 0) {
    INFO("Sensor last minute average read: " 
      + String(distanceLastMinute / samples)
      + " cm (samples: " + String(samples)
      + ", handleSensor calls: " + String(sensorCallsThisMinute)
      + ")");
  } else if (!alarmArmed) {
    INFO("Distance measuring OFF.");
  } else {
    WARN("This shouldn't happen. If you see this, something is not working as it should.");
  }

  distanceLastMinute = 0;
  sensorCallsThisMinute = 0;
  samples = 0;
}



  // Main guy here
  if(millis() - lastCheckTime >= SENSOR_INTERVAL) {
    lastCheckTime = millis();
    if(alarmArmed) {
      handleSensor();
    }
  }

   // Here was func for toggling buzzer. discontinued.

  handleTelegramSending();

  if (!alarmTriggered && millis() - lastBotCheck > BOT_CHECK_INTERVAL) {
    lastBotCheck = millis();
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if(numNewMessages) {
      handleNewMessages(numNewMessages);
    }
  }
}




