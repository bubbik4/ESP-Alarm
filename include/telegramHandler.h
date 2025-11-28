#pragma once
#include <Arduino.h>

void initTelegram();
// Kolejka wiadomości do wysłania
void queueTelegramMessage(const String &chatId, const String &msg, bool force = false);

// Obsługa wysyłania i odbierania
void handleTelegramSending();
void handleNewMessages(int numNewMessages);
void handleTelegramUpdates();
