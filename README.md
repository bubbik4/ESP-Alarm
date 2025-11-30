# AlarmESP-remake

System alarmowy oparty na ESP8266 z:
- czujnikiem ultradźwiękowym (HC-SR04),
- powiadomieniami Telegram,
- loggerem TCP,
- aktualizacjami OTA,
- buzzerem sygnalizacyjnym.

## Wersje

Aktualna wersja: **v1.2.0**

Dokumentacja dla tej wersji: [docs/AlarmESPv2.md](docs/AlarmESP-remake.md)

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