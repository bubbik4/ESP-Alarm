# AlarmESP-remake

System alarmowy oparty na ESP8266 z:
- czujnikiem ultradźwiękowym (HC-SR04),
- powiadomieniami Telegram,
- loggerem TCP,
- aktualizacjami OTA,
- buzzerem sygnalizacyjnym.

Sczegółowa dokumentacja projektu:
[docs/AlarmESPv2.md](docs/AlarmESP-remake.md)

## Struktura projektu

```text
AlarmESPv2/
├── src/
│   ├── main.cpp              # Punkt wejścia programu
│   ├── sensorHandler.cpp     # Obsługa czujnika i logiki alarmu
│   ├── logger.cpp            # Logger TCP i LED
│   ├── handleTelegram.cpp    # Komunikacja z Telegramem
│   └── otaHandle.cpp         # Aktualizacja OTA
│
├── include/
│   ├── sensorHandler.h
│   ├── logger.h
│   ├── telegramHandler.h
│   ├── otaHandle.h
│   ├── wifiAuth.h            # Dane Wi-Fi
│   └── telegramBotID.h       # Token bota i chat_id
│
└── docs/
    └── AlarmESPv2.md         # Dokumentacja

```