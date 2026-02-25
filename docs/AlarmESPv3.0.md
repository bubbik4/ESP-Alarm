# AlarmESP-MQTT - Dokumentacja Projektu

> **Wersja dokumentacji:** 3.0
> **Dotyczy wersji firmware:** v3.0
> **Data:** 2026-02-25

## Spis treści
1. [Opis projektu](#opis-projektu)
2. [Zmiany w wersji v3.0](#zmiany-w-wersji-v3.0)
3. [Architektura](#architektura)
4. [Moduły i pliki](#moduły-i-pliki)
6. [Tematy MQTT](#tematy-mqtt)
7. [Sygnalizacja LED](#sygnalizacja-led)

---

## Opis projektu
**AlarmESP-MQTT** to sensor dla systemu Smart Home. W przeciwieństwie do wersji v1, to urządzenie nie podejmuje decyzji o wysyłaniu powiadomień. Jego zadaniem jest:

1. Wykryć zmianę odległości (HC-SR04).
2. Wysłać raport do brokera MQTT.
3. Odzwierciedlić stan systemu (otrzymany z MQTT) na pasku LED.

---

## Zmiany w wersji v3.0

Wersja **v3.0** przygotowywana jest do bycia finalnym wydaniem programu ESP-Alarm dla urządzenia Seed Studio Xiao ESP32C6.
Na ten moment jest *szkieletem*, absolutnie nie jest to jeszcze finalne wydanie.  

Różni się ona od poprzednich wersji tym, że w tej implementacji skupiam się na obiektowym podejściu.

---

## Moduły i pliki

1. `secrets.h` i `secrets_template.h`
    - Tutaj trzymane są dane poufne, jak SSID, hasła. Plik `template` jest guidelinem, do tworzenia własnego `secrets.h`.
    - `secrets.h` jest w mojej implementacji symlinkiem do pliku istniejącego poza repozytorium.
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

### !Niezaimplementowane w wersji v3.0
<!-- | Kolor | Stan | Opis |
| :--- | :--- | :--- |
| 🔵 (Animacja) | Boot | Uruchamianie systemu. |
| 🟡 (Miganie) | WiFi Lost | Błąd połączenia lub tryb konfiguracji AP. |
| 🟢 (Stały/Zanikający) | Disarmed | System czuwa, ale nie alarmuje. Gaśnie po minucie. |
| 🔴 (Pulsowanie) | Armed | System uzbrojony. Wykrycie ruchu uruchomi alarm. |
| 🚨 (Czerwony/Niebieski) | Alarm | Wykryto intruza! | -->

---

## API (Funkcje C++)
