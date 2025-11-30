# AlarmESPv2 - Dokumentacja Projektu

> **Wersja dokumentacji:** 1.2.0
> **Dotyczy wersji firmware:** v1.2.0
> **Data:** 2025-11-30

## Spis treści
1. [Opis projektu](#opis-projektu)
2. [Struktura projektu](#struktura-projektu)
3. [Moduły i pliki](#moduły-i-pliki)
4. [Pierwsze uruchomienie](#pierwsze-uruchomienie)
5. [API](#api)
6. [Notatki i uwagi](#notatki-i-uwagi)

---

## Opis projektu
**AlarmESPv2** to system alarmowy oparty na ESP8266, wykorzystujący ultradźwiękowy czujnik odległości do detekcji otwarcia drzwi lub innych przeszkód.
Projekt integruje:
- Powiadomienia Telegram,
- Logger TCP,
- Aktualizacje OTA (Over-The-Air),
- Buzzer sygnalizacyjny,
- Prosty mechanizm statystyk sensora (średnia odległość, liczba pomiarów, wywołań `loop()`).
- WiFi manager

---

## Struktura projektu

```text
AlarmESPv2/
├── src/
│   ├── main.cpp              # Punkt wejścia programu
│   ├── sensorHandler.cpp     # Obsługa czujnika i logiki alarmu
│   ├── logger.cpp            # Logger TCP, statystyki minutowe, LED
│   ├── handleTelegram.cpp    # Komunikacja z Telegramem
│   ├── otaHandle.cpp         # Aktualizacja OTA
│   └── wifiHandler.cpp       # Obsługa połączenia WiFi i konfiguracji, WiFiManager
│
├── include/
│   ├── sensorHandler.h
│   ├── logger.h
│   ├── telegramHandler.h
│   ├── otaHandle.h
│   ├── wifiHandler.h         
│   └── telegramBotID.h       # Token bota i chat_id
│
└── docs/
    └── AlarmESPv2.md         # Dokumentacja
```

---



## Moduły i pliki

### 1. `main.cpp`
- Inicjalizacja wszystkich modułów:
    - `initSensor()`, `initLogger()`, `initOTA()`, `initTelegram()`
- pętla główna `loop()` obsługująca:
    - OTA, Logger, Sensor, Telegram

---

### 2. `sensorHandler.cpp/h`
- Obsługa czujnika HC-SR04:
    - `getFilteredDistance()` - średnia z 5 ostatnich odczytów (filtrowanie szumów)
    - `handleAlarm(distance)` - logika alarmu (otwarte/zamknięte drzwi)
    - W razie 10 kolejnych nieudanych odczytów - restart (`ESP.restart()`)
- Histereza alarmu:
    - logika alarmyu wykorzystuje dwa progi (`OPEN_THRESHOLD`, `CLOSE_THRESHOLD`), aby zapobiec migotaniu stanu na granicy.
- Buzzer:
    - `buzzerISR()` - sygnalizacja miganiem za pomocą `Ticker`
- Powiadomienia:
    - `sendAlarmNotification(distance)` - wysyła wiadomość Telegram
- Statystyki minutowa:
    - Zbierane są:
        - liczba wywołań pomiaru w danej minucie,
        - średnia odległość w danej minucie
    - Aktualizowane są w `sensorStatsTick(lastDistance)`, wywoływanym z `handleSenor()`.
    - Raz na minutę logger pobiera dane przez `sensorStatsGet(...)` i wypisuje je w logach.
- Publiczne funkcje:
```cpp
void initSensor();
void handleSensor();
bool isAlarmTriggered();
float getLastDistance();
void sensorStatsTick(float lastDistance);
void sensorStatsGet(int &calls, float &avgDistanceCm);
```

---

### 3. `logger.cpp/h`
- Logger TCP (port 7777) dla zdalnego podglądu logów
- Podłączenie klienta:
    - czyści terminal sekwencją ANSI (\033[2J\033[H)
    - wypisuje banner `AlarmESP-remake LOG console!`
    - wypisuje aktualny uptime w minutach
- Format logów: `[TIMESTAMP][LEVEL]MESSAGE`, gdzie:
    - `TIMESTAMP` w formacie `YYYY-MM-DD HH:MM:SS` (z `getTimestamp()`)
    - `LEVEL` - `LOG()`, `INFO()`, `WARN()`, `ERROR()`, `ALERT()`
    - kolorowanie poziomów za pomocą ANSI
- Makra logowania (`logger.h`):
    - `RAW(msg)`
    - `ALERT(msg)`
    - `LOG(msg)`
    - `INFO(msg)`
    - `WARN(msg)`
    - `ERROR(msg)`
- Przykładowy LOG minutowy:
```text
2025-xx-xx 12:34:56  [INFO]  Minute stats -> uptime=42 min, loopTicks=150000, sensorCalls=300, avgDistance=12.3 cm
```
- Uptime:
    - ilczony lokalnie z wykorzystaniem `millis()`:
        - zmienna `startMills`
        - funkcja pomocnicza `getUptimeMinutes()`
---

### 4. `handleTelegram.cpp/h`
- Inicjalizacja bota:
    - `initTelegram()` - konfiguruje klienta Telegram, ustawia token, interwały, itp.
- Kolejka wiadomości:
    - `queueTelegramMessage(chatId, msg)` - dodaje wiadomości do wysyłki
    - `handleTelegramSending()`           - Wysyła wiadomości z retry i cooldownem, integruje się z loggerem
- Obsługa przychodzących komend:
    - komendy bota:
        - `/alarm` - uzbrojenie / rozbrojenie alarmu
        - `/status` - status:
            - czy alarm uzbrojony,
            - ostatni pomiar odległości,
        - `/start`, `/help` - podstawowa informacja o bocie
- Autoryzacja wg. `ALLOWED_CHAT_ID` - wiadomości z innych czatów są ignorowane
- `handleTelegramUpdates()` - polling nowych wiadomości

---

### 5. `otaHandle.cpp/h`
- OTA (Over-The-Air) aktualizacje oprogramowania:
    - `initOTA()`:
        - ustawia `ArduinoOTA.setHostname(...);`
        - rejestruje callbacki:
            - onStart,
            - onEnd,
            - onProgress,
            - onError,
        - każdy callback loguje zdarzenia przez logger.
    - `handleOTA()` - cykliczna obsługa OTA w pętli

---

### 6. `wifiHandler.cpp/h`
 - Zastępuje plik wifiAuth.h i logikę łączenia w `main.cpp`
 - `initWiFiManager()`:
    - Uruchamia **WiFiManager**. Przy pierwszym starcie lub po utracie danych, inicjuje Captive Portal (AP o nazwie `AlarmESP-Setup`).
    - Ustawia timeout konfiguracyjny (180s).
    - Konfiguruje automatyczne ponawianie połączenia (`WiFiAutoReconnect(true)`).
- `configModeCallback()`:
    - Wywoływany, gdy ESP wchodzi w tryb AP.
    - Uruchamia nieblokujące miganie LED za pomocą `Ticker`, aby wizualnie zasygnalizować potrzebę konfiguracji.
- `handleWiFiConnection()`:
    - Sprawdza status połączenia w pętli `loop()` (co 30 sekund).
    - Loguje ostrzeżenia o utracie połączenia

---

### 7. `telegramBotID.h`
Plik z danymi do bota Telegrama
```cpp
#define BOT_TOKEN "token_bota"
extern const long ALLOWED_CHAT_ID;
```

---

## Pierwsze uruchomienie
- Po pierwszym wgraniu firmware urządzenie wejdzie w tryb konfiguracji:
 
### 1. Dioda LED zacznie szybko migać
### 2. Połącz się do sieci WiFi o nazwie `AlarmESP-Setup`.
### 3. Po połączeniu uruchomi się przeglądarka ze stroną konfiguracyjną. Wybierz swoją sieć i podaj hasło.
### 4. Urządzenie zapisze dane i skonfiguruje się w tryb alarmu.
### 5. Skopiuj i ustaw `BOT_TOKEN` i `ALLOWED_CHAT_ID` w `telegramBotID.h`.
### 6. Skonfiguruj progi alarmu w `sensorHandler.cpp`:
    - `OPEN_THRESHOLD`  - odległość uznawana za otwarcie
    - `CLOSE_THRESHOLD` - odległość uznawana za zamknięcie
### 7. Opcjonalnie zmień port TCP logów w  `logger.cpp`

---

## API

| Funkcja                               | Plik             | Opis                                     |
| ------------------------------------- | ---------------- | ---------------------------------------- |  
| `initSensor()`                        | sensorHandler.h  | Inicjalizacja pinów i buzzera            |
| `handleSensor()`                      | sensorHandler.h  | Odczyt czujnika i obsługa alarmu         |
| `isAlarmTriggered()`                  | sensorHandler.h  | Zwraca stan alarmu                       |
| `getLastDistance()`                   | sensorHandler.h  | Zwraca ostatnią zmierzoną odległość      |
| `initTelegram()`                      | handleTelegram.h | Inicjalizacja bota Telegram              |
| `queueTelegramMessage(chatId, msg)`   | handleTelegram.h | Dodaje wiadomość do kolejki              |
| `handleTelegramSending()`             | handleTelegram.h | Wysyła kolejkę wiadomości                |
| `handleTelegramUpdates()`             | handleTelegram.h | Polling nowych wiadomości                |
| `initOTA()`                           | otaHandle.h      | Inicjalizacja OTA                        |
| `handleOTA()`                         | otaHandle.h      | Obsługa OTA w pętli                      |
| `LOG()/INFO()/WARN()/ERROR()/ALERT()` | logger.h         | Funkcje logowania TCP i LED              |
| `initWiFiManager()`                   | wifiHandler.h    | Uruchamia pk lub łączy z zapisaną siecią |
| `handleWiFiConenction()`              | wifiHandler.h    | Monitoruje status połączenia w tle       |
| `getWiFiQuality()`                    | wifiHandler.h    | Zwraca jakość sygnału Wi-Fi (RSSI)       | 

---

## Notatki i uwagi
- Tryb konfiguracji: Sygnalizowany przez szybkie miganie diody LED.
- Cooldown Telegram: 30 sekund (`ALARM_COOLDOWN`) - chroni przed spamem powiadomień.
- Filtr odczytu czujnika: z 5 pomiarów czujnika.
- Buzzer sterowany przez Ticker co 100ms; miga gdy alarm jest aktywny.
- Komendy Telegram obsługiwane tylko dla `ALLOWED_CHAT_ID`.
- Logger TCP:
    - może obsługiwać wielu klientów jednocześnie, port domyślny: 7777
    - wypisuje wszystkie logi na Serial i na połączonego klienta TCP,
    - co minutę loguje statystyki:
        - uptime,
        - liczbę iteracji `loop()`,
        - liczbę odczytów sensora,
        - średnią odległość odczytaną przez sensor.
- OTA:
    - wymaga podłączenia ESP do sieci Wi-Fi,
    - podczas aktualizacji odpowiednie eventy są logowane (start, postęp, błąd).
- Plik z hasłami (`wifiAuth.h`) został usunięty. `telegramBotID.h` powinnien być wyłączony z publicznego repozytorium (`.gitignore`).

---