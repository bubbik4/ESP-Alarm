# AlarmESP-MQTT

System alarmowy oparty na ESP8266, zoptymalizowny pod kątem stabilności połączenia MQTT w ekosystemie Smart Home (Node-RED).

## Główne funkcje:
- **Detekcja:** Czujnik ultradźwiękowy (HC-SR04) wykrywający otwarcie drzwi/obecność.
- **Stabilność:** Autonomiczny system naprawy połączenia - wykrywanie "Zombie WiFi" i twardy reset radia.
- **Komunikacja:** Protokół MQTT - publikowanie stanu i alarmów, nasłuchiwanie komend uzbrajania.
- **Sygnalizacja:** Pasek LED WS2812b (*NeoPixel*) sygnalizujący stany (Uzbrojony, Rozbrojony, Alarm, Błąd sieci).
- **Zarządzanie:** WiFiManager (konfiguracja sieci bez hardcodowania), OTA (aktualizacje bezprzewodowe).

## Wersje

Aktualna wersja firmware: **v1.4.1-mqtt**

Dokumentacja szczegółowa: [docs/AlarmESP-remake.md](docs/AlarmESP-remake.md)

## Struktura projektu

```text
AlarmESP-MQTT/
├── src/
│   ├── main.cpp              # Punkt wejścia, pętla główna
│   ├── sensorHandler.cpp     # Obsługa HC-SR04, logika wyzwalania
│   ├── mqttHandler.cpp       # Klient MQTT (PubSubClient) / strażnik połączenia sieciowego
│   ├── ledHandler.cpp        # Obsługa paska LED (Adafruit NeoPixel)
│   ├── otaHandle.cpp         # Aktualizacja OTA
│   └── wifiHandler.cpp       # Obsługa WiFi i Captive Portal
│
├── include/
│   ├── (pliki nagłówkowe .h odpowiadające modułom wyżej)
│
└── docs/
    └── AlarmESP-remake.md    # Dokumentacja techniczna
