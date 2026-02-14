#include "app_data.h"
#include <Preferences.h>
#include "ui/ui.h"

AppDataManager AppData;
Preferences preferences;

void AppDataManager::begin() {
    preferences.begin("mixer-app", false);
    loadState();
    
    // Initialize RS485
    Serial1.begin(115200, SERIAL_8N1, 43, 44); // RX=43, TX=44
}

void AppDataManager::saveState() {
    preferences.putInt("mus_v", music_volume);
    preferences.putInt("mic_v", mic_volume);
    preferences.putInt("main_f", main_fader);
    preferences.putBool("mus_r", music_relay_state);
    preferences.putBool("mic_r", mic_relay_state);
}

void AppDataManager::loadState() {
    music_volume = preferences.getInt("mus_v", 80);
    mic_volume = preferences.getInt("mic_v", 50);
    main_fader = preferences.getInt("main_f", 100);
    music_relay_state = preferences.getBool("mus_r", true);
    mic_relay_state = preferences.getBool("mic_r", true);
}

void AppDataManager::sendUpdate() {
    sendJSON();
    dirty = true;  // Will be saved by throttled save in main loop
}

void AppDataManager::sendJSON() {
    StaticJsonDocument<200> doc;
    
    // Calculated volumes based on Main Fader
    int final_mus_vol = (music_volume * main_fader) / 100;
    int final_mic_vol = (mic_volume * main_fader) / 100;

    doc["mv"] = final_mus_vol;
    doc["cv"] = final_mic_vol; // c for Channel (Microphone)
    doc["mr"] = music_relay_state ? 1 : 0;
    doc["cr"] = mic_relay_state ? 1 : 0;
    
    serializeJson(doc, Serial1);
    Serial1.println(); // Send newline
    
    // Debug: Echo to Serial Monitor
    Serial.print("RS485 TX: ");
    serializeJson(doc, Serial);
    Serial.println();
}

void AppDataManager::handleIncomingData(Stream &serial) {
    if (serial.available()) {
        String input = serial.readStringUntil('\n');
        input.trim();
        
        // Check for '?' or JSON command "get"
        if (input == "?" || input.indexOf("\"cmd\":\"get\"") >= 0) {
            sendUpdate();
        }
    }
}

void AppDataManager::syncUI() {
    // 1. Update Sliders
    if (ui_Slider1) lv_slider_set_value(ui_Slider1, mic_volume, LV_ANIM_OFF);
    if (ui_Slider2) lv_slider_set_value(ui_Slider2, music_volume, LV_ANIM_OFF);
    if (ui_Slider3) lv_slider_set_value(ui_Slider3, main_fader, LV_ANIM_OFF); // Update Main Fader
    
    // 2. Update Switches (Relays)
    // We toggle them inversely (One Checked, One Unchecked)
    
    // Mic Relay
    if (mic_relay_state) {
        if(ui_Button3) lv_obj_clear_state(ui_Button3, LV_STATE_CHECKED);
        if(ui_Button4) lv_obj_add_state(ui_Button4, LV_STATE_CHECKED);
    } else {
        if(ui_Button3) lv_obj_add_state(ui_Button3, LV_STATE_CHECKED);
        if(ui_Button4) lv_obj_clear_state(ui_Button4, LV_STATE_CHECKED);
    }

    // Music Relay
    if (music_relay_state) {
        if(ui_Button1) lv_obj_clear_state(ui_Button1, LV_STATE_CHECKED);
        if(ui_Button2) lv_obj_add_state(ui_Button2, LV_STATE_CHECKED);
    } else {
        if(ui_Button1) lv_obj_add_state(ui_Button1, LV_STATE_CHECKED);
        if(ui_Button2) lv_obj_clear_state(ui_Button2, LV_STATE_CHECKED);
    }
}
