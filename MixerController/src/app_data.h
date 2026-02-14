#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class AppDataManager {
public:
    int music_volume = 80;
    int mic_volume = 50;
    int main_fader = 100; // 0-100 percentage
    bool music_relay_state = true;
    bool mic_relay_state = true;
    bool power_sensing_enabled = true;  // Auto on/off via USB charger on DI0
    bool dirty = false;  // Set true when values change, cleared after save

    void begin();
    void saveState();
    void loadState();
    void updateFromUI(const char* event_type, int value);
    void sendUpdate();
    void handleIncomingData(Stream &serial);
    void syncUI(); // Updates UI widgets from current variables

private:
    void sendJSON();
};

extern AppDataManager AppData;
