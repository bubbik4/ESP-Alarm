# AlarmESPv2 - Dokumentacja Projektu

## Spis treści
1. [Opis projektu](#opis-projektu)
2. [Struktura projektu](#struktura-projektu)
3. [Moduły i pliki](#moduły-i-pliki)
4. [Konfiguracja](#konfiguracja)
6. [API](#api)
7. [Notatki i uwagi](#notatki-i-uwagi)

---

## Opis projektu
**AlarmESPv2** to system alarmowy oparty na ESP8266, wykorzystujący ultradźwiękowy czujnik odległości do detekcji otwarcia drzwi lub innych przeszkód.
Projekt integruje:
- Powiadomienia Telegram,
- Logger TCP,
- Aktualizacje OTA (Over-The-Air),
- Buzzer sygnalizacyjny.

---

## Struktura projektu

AlarmESPv2/
│
├── src/
│ ├── main.cpp # Punkt wejścia programu
│ ├── sensorHandler.cpp # Obsługa czujnika i logiki alarmu
│ ├── logger.cpp # Logger TCP i LED
│ ├── handleTelegram.cpp# Komunikacja z Telegramem
│ └── otaHandle.cpp # Aktualizacja OTA
│
├── include/
│ ├── sensorHandler.h
│ ├── logger.h
│ ├── telegramHandler.h
│ ├── otaHandle.h
│ ├── wifiAuth.h # Dane Wi-Fi
│ └── telegramBotID.h # Token bota i chat_id
│
└── docs/
└── AlarmESPv2.md # Dokumentacja

---

## Moduły i pliki

### 1. `main.cpp`
- Inicjalizacja wszystkich modułow:
    - `initSensor()`, `initLogger()`, `initOTA()`, `initTelegram()`
- pętla głowna `loop()` obsługująca:
    - OTA, Logger, Sensor, Telegram

---

### 2. `sensorHandler.cpp/h`
- Obsługa czujnika HC-SR04:
    - `getFilteredDistance()` - średnia z 5 ostatnich odczytów
    - `handleAlarm(distance)` - logika alarmu (otwarte/zamknięte drzwi)
- Buzzer:
    - `buzzerISR()` - sygnalizacja miganiem
- Powiadomienia:
    - `sendAlarmNotification(distance)` - wysyła wiadomość Telegram
- Publiczne funkcje:
```cpp
void initSensor();
void handleSensor();
bool isAlarmTriggered();
float getLastDistance();
```

---

### 3. `logger.cpp/h`
- Logger TCP (port 7777) dla zdalnego podglądu logów
- Format logów: `[TIMESTAMP][LEVEL]MESSAGE`
- Funkcje:
    - `LOG()`, `INFO()`, `WARN()`, `ERROR()`, `ALERT()`
    - `getTimeStamp()`
    - `shortBlink(int interval)` - szybkie miganie LED dla debugowania

---

### 4. `handleTelegram.cpp/h`
- Inicjalizacja bota:
    - `initTelegram()`
- Kolejka wiadomości:
    - `queueTelegramMessage(chatId, msg)` - dodaje wiadomości do wysyłki
    - `handleTelegramSending()`           - Wysyła wiadomości z retry i cooldownem
- Obsługa przychodzących komend:
    - `/alarm`, `/status`, `/start`, `/help`
- Autoryzacja wg. `ALLOWED_CHAT_ID`
- `handleTelegramUpdates()` - polling nowych wiadomości

---

### 5. `otaHandle.cpp/h`
- OTA (Over-The-Air) aktualizacje oprogramowania:
    - `initOTA()`   - konfiguruje hostname i callbacki logujące
    - `handleOTA()` - cykliczna obsługa OTA w pętli

---

### 6. `wifiAuth.h`
```cpp
#define WIFI_SSID "nazwa_sieci"
#define WIFI_PASSWORD "haslo_sieci"
```

---

### 7. `telegramBotID.h`
```cpp
#define BOT_TOKEN "token_bota"
extern const long ALLOWED_CHAT_ID
```

---

## Konfiguracja
 
### 1. Skopiuj i wypełnij dane WiFi w `wifiAuth.h`.
### 2. Skopiuj i ustaw `BOT_TOKEN` i `ALLOWED_CHAT_ID` w `telegramBotID.h`.
### 3. Skonfiguruj progi alarmu w `sensorHandler.cpp`:
    - `OPEN_THRESHOLD`  - odległość uznawana za otwarcie
    - `CLOSE_THRESHOLD` - odległość uznawana za zamknięcie
### 4. Opcjonalnie zmień port TCP logów w  `logger.cpp`

---

## API

| Funkcja                               | Plik             | Opis                                |
| ------------------------------------- | ---------------- | ----------------------------------- |
| `initSensor()`                        | sensorHandler.h  | Inicjalizacja pinów i buzera        |
| `handleSensor()`                      | sensorHandler.h  | Odczyt czujnika i obsługa alarmu    |
| `isAlarmTriggered()`                  | sensorHandler.h  | Zwraca stan alarmu                  |
| `getLastDistance()`                   | sensorHandler.h  | Zwraca ostatnią zmierzoną odległość |
| `initTelegram()`                      | handleTelegram.h | Inicjalizacja bota Telegram         |
| `queueTelegramMessage(chatId, msg)`   | handleTelegram.h | Dodaje wiadomość do kolejki         |
| `handleTelegramSending()`             | handleTelegram.h | Wysyła kolejkę wiadomości           |
| `handleTelegramUpdates()`             | handleTelegram.h | Polling nowych wiadomości           |
| `initOTA()`                           | otaHandle.h      | Inicjalizacja OTA                   |
| `handleOTA()`                         | otaHandle.h      | Obsługa OTA w pętli                 |
| `LOG()/INFO()/WARN()/ERROR()/ALERT()` | logger.h         | Funkcje logowania TCP i LED         |


---

## Notatki i uwagi
- Cooldown Telegram: 30 sekund (`ALARM_COOLDOWN`)
- Filtr odczytu czujnika: z 5 pomiarów
- Buzzer sterowany przez Ticker co 100ms
- Komendy Telegram obsługiwane tylko dla `ALLOWED_CHAT_ID`
- Logger TCP może obsługiwać wielu klientów jednocześnie
- OTA wymaga podłączenia ESP do sieci Wi-Fi

---