#include "telegramHandler.h"
#include "logger.h"
#include "telegramBotID.h"
#include "sensorHandler.h"
#include <UniversalTelegramBot.h>

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

struct TgMessage {
    String chatId;
    String text;
};

bool pendingTelegram = false;
TgMessage pendingMsg;
unsigned long telegramLastAttempt = 0;
const unsigned long TELEGRAM_RETRY_INTERVAL = 1000; // 1s
uint8_t telegramRetryCount = 0;
const uint8_t TELEGRAM_MAX_RETRIES = 3;

uint32_t msgCounter = 0;
unsigned long lastBotCheck = 0;
const unsigned long BOT_CHECK_INTERVAL = 5000; // 5s

static uint32_t lastSentMsgId = 0;
static unsigned long lastSentMsgTime = 0;

void queueTelegramMessage(const String &chatId, const String &msg, bool force) {
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

void handleNewMessages(int numNewMessages) {
  for(int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from = bot.messages[i].from_name;

    LOG("Telegram message from: " + from + " (" + chat_id + "): " + text);
    shortBlink();

    String reply;

    if(chat_id.equals(String(ALLOWED_CHAT_ID))) {
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

        if(!alarmArmed && isAlarmTriggered()) {
          resetAlarm();
          INFO("Alarm disarmed via Telegram. Buzzer turned off.");
        }

      } else if( text == "/status") {
        String state = alarmArmed ? "UZBROJONY" : "ROZBROJONY";
        reply =   "AlarmESP-remake status report:\n";
        reply +=  "• Alarm state: *" + state + "*\n";
        reply +=  "• Last read: " + String(getLastDistance(), 1) + " cm";

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

void handleTelegramUpdates() {
    if(isAlarmTriggered()) {
        // if alarm is active, do not use bot
        return;
    }

    if (!isAlarmTriggered() && millis() - lastBotCheck > BOT_CHECK_INTERVAL) {
        lastBotCheck = millis();
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        if(numNewMessages) {
          handleNewMessages(numNewMessages);
        }
    }
}

void initTelegram() {
    secured_client.setInsecure();
    LOG("Telegram client configured (insecure mode)");
}
