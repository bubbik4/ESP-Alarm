# AlarmESP-MQTT - Dokumentacja Projektu

> **Wersja dokumentacji:** 2.1.0
> **Dotyczy wersji firmware:** v1.4.1-mqtt
> **Data:** 2026-01-20

## Spis treści
1. [Opis projektu](#opis-projektu)
2. [Zmiany w wersji 1.4.1-mqtt](#zmiany-w-wersji-v141-mqtt)
3. [Architektura](#architektura)
4. [Moduły i pliki](#moduły-i-pliki)
5. [Pierwsze uruchomienie](#pierwsze-uruchomienie)
6. [Tematy MQTT](#tematy-mqtt)
7. [Sygnalizacja LED](#sygnalizacja-led)

---

## Opis projektu
**AlarmESP-MQTT** to sensor dla systemu Smart Home. W przeciwieństwie do wersji v1, to urządzenie nie podejmuje decyzji o wysyłaniu powiadomień. Jego zadaniem jest:

1. Wykryć zmianę odległości (HC-SR04).
2. Wysłać raport do brokera MQTT.
3. Odzwierciedlić stan systemu (otrzymany z MQTT) na pasku LED.

Logika powiadomień (Telegram) została przeniesiona do **Node-RED**.

---

## Zmiany w wersji v1.4.1-mqtt

- **Usunięto logger TCP:** zastąpiono go lekkimi makrami (`Serial.println()`) dla oszczędności pamięci i cykli CPU.
- **Usunięto statystyki:** wycięto zbędne zliczanie średnich wywołań pętli co minutę.
- **Uproszczony WiFi Handler:** moduł służy teraz tylko do wstępnej konfiguracji. Nie monitoruje już połączenia w tle, robi to [MQTT Handler](#2-mqtthandlercpph-zastępuje-handletelegram).

---

## Moduły i pliki

### 1. `main.cpp`

- Inicjalizacja: `initLeds()`, `initLogger()`, `initWiFiManager()`, `initOTA()`, `initSensor()`, `initMQTT()`.
- Główna pętla `loop()` sterująca wszystkimi podsystemami.

### 2. `mqttHandler.cpp/h`

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
- Zawiera "Histerezę" (progi Open/Close) zapobiegającą fałszywym alarmom na granicy zasięgu.

### 5. `wifiHandler.cpp/h` & `otaHandle.cpp/h`

- Standardowa obsługa WiFiManager (Captive Portal) oraz aktualizacji OTA.
- Funkcja `checkResetButton()` pozwala na fizyczny reset ustawień sieciowych.

---

## Tematy MQTT

Urządzenie komunikuje się z serwerem (Node-RED) używając następujących tematów:

| Temat | Kierunek | Treść | Opis |
| :--- | :---: | :--- | :--- |
| `dom/alarm/set` | Odbiór (SUB) | `ON` / `OFF` | Uzbraja lub rozbraja alarm (zmienia też kolor LED). |
| `dom/alarm/status` | Wysyłka (PUB) | `ARMED` / `DARMED` | Potwierdzenie zmiany stanu dla Node-RED. |
| `dom/alarm/trigger` | Wysyłka (PUB) | `ALARM! Dist: X cm` | Wysyłane natychmiast po wykryciu intruza. |
| `dom/alarm/LWT` | Wysyłka (LWT) | `ONLINE` / `OFFLINE` | Last Will & Testament (monitorowanie dostępności). |


---

## Sygnalizacja LED

Pasek LED jest głównym interfejsem dla użytkownika w domu:

| Kolor | Stan | Opis |
| :--- | :--- | :--- |
| 🔵 (Animacja) | Boot | Uruchamianie systemu. |
| 🟡 (Miganie) | WiFi Lost | Błąd połączenia lub tryb konfiguracji AP. |
| 🟢 (Stały/Zanikający) | Disarmed | System czuwa, ale nie alarmuje. Gaśnie po minucie. |
| 🔴 (Pulsowanie) | Armed | System uzbrojony. Wykrycie ruchu uruchomi alarm. |
| 🚨 (Czerwony/Niebieski) | Alarm | Wykryto intruza! |

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
| `forceNetworkRestart()` | mqttHandler | Wymusza twardy reset interfejsu WiFi. |
| `sendMQTTAlarm(dist)` | mqttHandler | Publikuje wiadomość o włamaniu. |
| `setLedState(state)` | ledHandler | Zmienia tryb świecenia paska LED. |
| `checkResetButton()` | sensorHandler | Sprawdza fizyczny przycisk resetu configu. |