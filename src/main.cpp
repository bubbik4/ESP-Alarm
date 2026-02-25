#include <Arduino.h>

#include "NetworkManager.h"
#include "OtaManager.h"

// --- class objects initialization ---
NetManager network(
    SSID_24_main,
    PASSWD_24_main,
    IPAddress(ALARM_ESP_IP_ADDR),
    IPAddress(WIFI_GATEWAY_IP),
    IPAddress(WIFI_SUBNET),
    IPAddress(WIFI_DNS)
);

OtaManager ota(ALARM_OTA_HOSTNAME, ALARM_OTA_PASSWORD);

// --- setup ---
void setup() {
    Serial.begin(115200);
    delay(2000);
    LOG("System starting...");

    network.connect();

    if(network.isConnected()) {
        ota.setup();
    }
}
// --- main loop ---
void loop() {
    ota.handle();
}
