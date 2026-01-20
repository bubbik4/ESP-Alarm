#include "mqttHandler.h"
#include "logger.h"
#include "sensorHandler.h"
#include "ledHandler.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

WiFiClient   espClient;
PubSubClient client(espClient);

const char* mqtt_server = "10.10.0.70"; // IP Maliny

// Tematy MQTT
const char* topic_set = "dom/alarm/set";
const char* topic_status = "dom/alarm/status";
const char* topic_trigger = "dom/alarm/trigger";
const char* topic_lwt = "dom/alarm/LWT"; 

static bool mqttWasConnected = false;

// Zmienne czasowe
unsigned long lastMqttAttempt = 0;
const unsigned long MQTT_RETRY_INTERVAL = 5000; // Próba co 5 sekund
static int connectionRetries = 0;
const int MAX_RETRIES = 3;

// Zmienne Offline (opcjonalne, jeśli chcesz zachować logikę 3 min ciszy)
bool inOfflineMode = false;           
unsigned long offlineStartTimer = 0;  
const unsigned long OFFLINE_DURATION = 60000; // Zmniejszyłem na 1 min dla testów!

// --- TA FUNKCJA ROBI ROBOTĘ ---
void forceNetworkRestart() {
    INFO("Nuclear Option: Turning WiFi OFF and ON again...");
    WiFi.disconnect(false);   
    delay(500);          
    WiFi.mode(WIFI_OFF); // Wyłączamy radio fizycznie
    delay(500);
    WiFi.mode(WIFI_STA); // Włączamy radio
    // Tu musisz podać swoje dane, albo te z wifiHandler.h
    // Jeśli używasz WiFiManager, to samo WiFi.begin() wystarczy (bez argumentów) często
    // Ale dla pewności lepiej wpisać:
    WiFi.begin(); 
    
    connectionRetries = 0; // Resetujemy licznik
}

void restoreSystemLeds() {
    if (isAlarmTriggered()) {
        setLedState(STATE_ALARM);    
    } else if (alarmArmed) {
        setLedState(STATE_ARMED);    
    } else {
        setLedState(STATE_DISARMED); 
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    INFO("MQTT Message [" + String(topic) + "]:" + msg);

    if(String(topic) == topic_set) {
        if(msg == "ON") {
            alarmArmed = 1;
            INFO("System ARMED via MQTT");
            client.publish(topic_status, "ARMED");
            setLedState(STATE_ARMED);
        } else if (msg == "OFF") {
            alarmArmed = 0;
            resetAlarm();
            INFO("System DISARMED via MQTT");
            client.publish(topic_status, "DARMED");
            setLedState(STATE_DISARMED);
        }
    }
}

bool tryConnect() {
    String clientId = "AlarmESP" + String(ESP.getChipId(), HEX);
    if(client.connect(clientId.c_str(), topic_lwt, 1, false, "OFFLINE")) {
        LOG("MQTT Connected!");
        client.publish(topic_lwt, "ONLINE"); 
        client.subscribe(topic_set);
        sendMQTTStatus();
        return true;
    } else {
        return false;
    }
}

void initMQTT() {
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);
    client.setKeepAlive(60);
    client.setSocketTimeout(60);
}

void handleMQTT() {
    // 1. Jeśli połączony - obsługa i wyjście
    if (client.connected()) {
        if (!mqttWasConnected) {
            mqttWasConnected = true;
            inOfflineMode = false; 
            connectionRetries = 0;
            INFO("MQTT connection restored!");
            restoreSystemLeds();
        }
        client.loop();
        return;
    }

    // 2. Jeśli brak WiFi - czekamy na wifiHandler
    if(WiFi.status() != WL_CONNECTED) {
        return; 
    }

    // 3. Cooldown (żeby nie spamować próbami co milisekundę)
    unsigned long now = millis();
    if (now - lastMqttAttempt < MQTT_RETRY_INTERVAL) {
        return; 
    }
    lastMqttAttempt = now;

    // 4. Próba połączenia
    if (tryConnect()) {
        mqttWasConnected = true;
        connectionRetries = 0;
    } else {
        // Nie udało się połączyć
        connectionRetries++;
        WARN("MQTT Failed. Attempt: " + String(connectionRetries) + "/" + String(MAX_RETRIES));


        // 5. JEŚLI TO 3. RAZ -> Restartuj WiFi
        if (connectionRetries >= MAX_RETRIES) {
            ERROR("MQTT Dead -> Restarting WiFi Interface!");
            
            // 3x Żółty (błąd)
            reportError(3, getColor(255, 200, 0));
            
            // --- NUCLEAR OPTION ---
            forceNetworkRestart(); 
            // ----------------------
            
            // Fail Safe ARM (jeśli chcesz)
            alarmArmed = 1;
            restoreSystemLeds();
        }
    }
}

void sendMQTTStatus() {
    if(client.connected()) {
        String state = alarmArmed ? "ARMED" : "DARMED";
        client.publish(topic_status, state.c_str());
    }
}

void sendMQTTAlarm(float distance) {
    if(client.connected()) {
        String msg = "ALARM! Dist: " + String(distance) + " cm";
        client.publish(topic_trigger, msg.c_str());
    }
}