#include "OtaManager.h"

OtaManager::OtaManager(const char* hostname, const char* password)
            : _hostname(hostname), _password(password) {}

void OtaManager::setup() {
    ArduinoOTA.setHostname(_hostname);
    ArduinoOTA.setPassword(_password);

    // Callbacks
    ArduinoOTA.onStart([]() {
        String type;
        if(ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {
            type = "filesystem";
        }

        LOGF("[OTA] Start updating", type);
    });

    ArduinoOTA.onEnd([]() {
        LOG("\n[OTA] End");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        LOGF("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        LOGF("[OTA] Err[%u]", error);
        if (error == OTA_AUTH_ERROR) LOG("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) LOG("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) LOG("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) LOG("Receive Failed");
        else if (error == OTA_END_ERROR) LOG("End Failed");
    });

    ArduinoOTA.begin();
    LOG("[OTA] Ready");
}

void OtaManager::handle() {
    ArduinoOTA.handle();
}