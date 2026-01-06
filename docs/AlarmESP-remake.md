# AlarmESP-MQTT - Dokumentacja Projektu

> **Wersja dokumentacji:** 2.0.0
> **Dotyczy wersji firmware:** v1.2.3 (MQTT)
> **Data:** 2026-01-06

## Spis treści
1. [Opis projektu](#opis-projektu)
2. [Architektura](#architektura)
3. [Moduły i pliki](#moduły-i-pliki)
4. [Pierwsze uruchomienie](#pierwsze-uruchomienie)
5. [Tematy MQTT](#tematy-mqtt)
6. [Sygnalizacja LED](#sygnalizacja-led)

---

## Opis projektu
**AlarmESP-MQTT** to aktuator/sensor dla systemu Smart Home. W przeciwieństwie do wersji v1, to urządzenie nie podejmuje decyzji o wysyłaniu powiadomień. Jego zadaniem jest:
1. Wykryć zmianę odległości (HC-SR04).
2. Wysłać raport do brokera MQTT.
3. Odzwierciedlić stan systemu (otrzymany z MQTT) na pasku LED.

Logika powiadomień (Telegram) została przeniesiona do **Node-RED**.

---

## Moduły i pliki

### 1. `main.cpp`
- Inicjalizacja: `initLeds()`, `initLogger()`, `initWiFiManager()`, `initOTA()`, `initSensor()`, `initMQTT()`.
- Synchronizacja czasu NTP.
- Główna pętla `loop()` sterująca wszystkimi podsystemami.

### 2. `mqttHandler.cpp/h` (Zastępuje handleTelegram)
- Obsługa biblioteki `PubSubClient`.
- **Logika:**
    - Łączy się z brokerem (adres IP zdefiniowany w kodzie lub configu).
    - Subskrybuje temat `dom/alarm/set` (nasłuchuje komend ON/OFF).
    - Publikuje statusy do `dom/alarm/status` oraz alarmy do `dom/alarm/trigger`.
    - Obsługuje `reconnect()` w przypadku utraty połączenia z brokerem.

### 3. `ledHandler.cpp/h` (Nowy moduł)
- Obsługa paska LED WS2812b (Adafruit NeoPixel).
- **Maszyna stanów LED (`SystemState`):**
    - `STATE_BOOT`: Niebieski "biegający" punkt.
    - `STATE_DISARMED`: Zielony stały (wygasza się po minucie).
    - `STATE_ARMED`: Czerwone pulsowanie (efekt oddychania).
    - `STATE_ALARM`: Policyjne miganie (Czerwony-Niebieski).
    - `STATE_WIFI_LOST`: Miganie na żółto.

### 4. `sensorHandler.cpp/h`
- Obsługa czujnika HC-SR04 z filtrowaniem (średnia z 5 pomiarów).
- Przycisk Resetu (GPIO D3): Przytrzymanie >3s resetuje ustawienia WiFi.
- **Zmiana względem v1:** Po wykryciu intruza nie wysyła Telegrama, lecz wywołuje `sendMQTTAlarm(distance)`.
- Zawiera "Histerezę" (progi Open/Close) zapobiegającą fałszywym alarmom na granicy zasięgu.

### 5. `wifiHandler.cpp/h` & `otaHandle.cpp/h`
- Standardowa obsługa WiFiManager (Captive Portal) oraz aktualizacji OTA, bez większych zmian względem v1.
- Funkcja `checkResetButton()` pozwala na fizyczny reset ustawień sieciowych.

---

## Tematy MQTT

Urządzenie komunikuje się z serwerem (Node-RED) używając następujących tematów:

| Temat | Kierunek | Treść | Opis |
| :--- | :---: | :--- | :--- |
| `dom/alarm/set` | Odbiór (SUB) | `ON` / `OFF` | Uzbraja lub rozbraja alarm (zmienia też kolor LED). |
| `dom/alarm/status` | Wysyłka (PUB) | `ARMED` / `DARMED` | Potwierdzenie zmiany stanu dla Node-RED. |
| `dom/alarm/trigger` | Wysyłka (PUB) | `ALARM! Dist: X cm` | Wysyłane natychmiast po wykryciu intruza. |

---

## Sygnalizacja LED

Pasek LED jest głównym interfejsem dla użytkownika w domu:

1. **Start:** 🔵 (Animacja startowa)
2. **Brak WiFi:** 🟡 (Miganie żółte)
3. **Rozbrojony:** 🟢 (Zielony przez 60s, potem gaśnie dla oszczędności)
4. **Uzbrojony:** 🔴 (Delikatne pulsowanie czerwone)
5. **ALARM:** 🚨 (Szybkie miganie Czerwony/Niebieski + Buzzer)

---

## Pierwsze uruchomienie

1. Wgraj firmware przez PlatformIO.
2. Dioda zacznie migać na niebiesko/żółto (tryb AP).
3. Połącz telefon z siecią WiFi `AlarmESP-Setup`.
4. W przeglądarce (192.168.4.1) skonfiguruj swoje domowe WiFi.
5. **Ważne:** Upewnij się, że w kodzie `mqttHandler.cpp` ustawiony jest poprawny adres IP Twojego brokera MQTT (Raspberry Pi), np. `10.10.0.70`.
6. Po restarcie pasek LED powinien zaświecić się na zielono (Stan domyślny: Rozbrojony).

---

## API (Funkcje C++)

| Funkcja | Moduł | Opis |
| :--- | :--- | :--- |
| `initMQTT()` | mqttHandler | Konfiguracja klienta i callbacków |
| `sendMQTTAlarm(dist)` | mqttHandler | Publikuje wiadomość o włamaniu |
| `setLedState(state)` | ledHandler | Zmienia tryb świecenia paska LED |
| `handleSensor()` | sensorHandler | Główna logika pomiarowa |
| `checkResetButton()` | sensorHandler | Obsługa fizycznego przycisku resetu |