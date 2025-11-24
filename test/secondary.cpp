#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

#include <BlynkAuth.h>
#include <BlynkSimpleEsp8266.h>

#include <setupConfig.h>
#include <wifiAuth.h>

void WiFiConnect();
void alarmTriggered();
void alarmCleared();
void alarmStateChange(bool currentState);
void mainWorker();
float sensorHandle();
bool sensorWatchdog();
void sensorDisarmed();


float distance = 0;
bool lastAlarmState = 0;
bool isArmed = 1;
// bool isBeeping = 0;
// unsigned long beepStartTime = 0;
// unsigned long beepDuration = 0;
int sensorFailCount = 0;
int alarmTreshold = 10;

int alarmPD = 0;

/*
bool nightModeEnabled = 1;
int nmStart = 22;
int nmEnd = 7;
*/

// int sensorFailCount = 0;

unsigned long lastWiFiCheck = 0;
unsigned long lastCheckTime = 0; // f'what? CHECK AND VERIFY


void OTASetup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  WiFiConnect();

  ArduinoOTA.onStart([]() {
  Serial.println("Start OTA upload");

  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA upload");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.setPassword((const char *)NULL); // brak hasła
  ArduinoOTA.begin();

  Serial.println("OTA ready");
}


void WiFiConnect() {
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void setup() {
  Serial.begin(115200);
  OTASetup();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzPin, OUTPUT);

  EEPROM.begin(512);
  //loadAlarmCounters();
}

void loop() {
  ArduinoOTA.handle();
  mainWorker();
  delay(100);
}

/* void startBeep(int duration) {
  if(!isBeeping) {
    tone(buzzPin, 1000);
    beepStartTime = millis();
    beepDuration = duration;
    isBeeping = true;
  }
} */

/* void updateBeep() {
  if(isBeeping && (millis() - beepStartTime >= beepDuration)) {
    noTone(buzzPin);
    isBeeping = false;
  }
} */

float sensorHandle() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  float duration = pulseIn(echoPin, HIGH, 30000);
  if(duration == 0) {
    Serial.println("No sensor answer | ignoring read");
    return -1; // No response
  } else {
    return (duration  * 0.0343) / 2;
  }
}

bool sensorWatchdog() {
  if (distance < 0) {
    sensorFailCount++;

    if(sensorFailCount >= 5) {
      ESP.restart();
    }
    return false;
  }
  sensorFailCount = 0;
  return true;
}

void sensorDisarmed() {
  if(lastAlarmState) {
    lastAlarmState = false;
    // Blynk.virtualWrite(V0, 0);
  }

  digitalWrite(buzzPin, LOW);
}

void alarmStateChange(bool currentState) {
  lastAlarmState = currentState;

  if(currentState) {
    alarmTriggered();
  } else {
    alarmCleared();
  }
}

void mainWorker() {
  lastCheckTime = millis();
  if (!isArmed) {
    sensorDisarmed();
    return;
  }

  distance = sensorHandle();

  if(!sensorWatchdog()) return;

  bool currentAlarmState = (distance > alarmTreshold && distance > 0);

  if(currentAlarmState != lastAlarmState) {
    alarmStateChange(currentAlarmState);
  }

}

void alarmTriggered() {
  alarmPD++;

  // Blynk.virtualWrite(V2, alarmPD);
  // Blynk.virtualWrite(V0, 1);
  
  for (int i = 0; i < 10; i++) {
    digitalWrite(buzzPin, HIGH);
    delay(100);
    digitalWrite(buzzPin, LOW);
    delay(100);
  }
}

void alarmCleared() {
  // Blynk.virtualWrite(V0, 0);
  digitalWrite(buzzPin, LOW);
}