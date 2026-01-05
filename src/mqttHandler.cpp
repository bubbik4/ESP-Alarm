#include "mqttHandler.h"
#include "logger.h"
#include "sensorHandler.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

const char* mqtt_server = "10.10.0.70"; // RPi IP

// tematy
const char* topic_set = "dom/alarm/set"; // Odbieranie: ON/OFF
const char* topic_status = "dom/alarm/status"; // Wysyłanie: ARM/DARM
const char* topic_trigger = "dom/alarm/trigger"; // Wysyłanie: Włamanie

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }

    INFO("MQTT Message [" + String(topic) + "]:" + msg);

    if(String(topic) == topic_set) {
        if(msg == "ON") {
            alarmArmed = 1;
            INFO("System ARMED via MQTT");
            client.publish(topic_status, "ARMED");
        } else if (msg == "OFF") {
            alarmArmed = 0;
            resetAlarm();
            INFO("System DISARMED via MQTT");
            client.publish(topic_status, "DARMED");
        }
    }
}

void reconnect() {
    if(!client.connected()) {
        //ID klienta
        String clientId = "AlarmESP" + String(ESP.getChipId(), HEX);
        if(client.connect(clientId.c_str())) {
            LOG("MQTT Connected!");
            client.subscribe(topic_set);
            sendMQTTStatus();
        } else {
            //
        }
    }
}

void initMQTT() {
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);
}

void handleMQTT() {
    if(!client.connected()) {
        static unsigned long lastReconnectAttempt = 0;
        unsigned long now = millis();
        if(now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            reconnect();
        }
    } else {
        client.loop();
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