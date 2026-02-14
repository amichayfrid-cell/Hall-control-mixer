#include "ui/ui.h"
#include "app_data.h"

extern "C" {

void mic_V(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    AppData.mic_volume = value;
    AppData.sendUpdate();
}

void music_V(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    AppData.music_volume = value;
    AppData.sendUpdate();
}

void main_Volume(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    AppData.main_fader = value;
    AppData.sendUpdate();
}

void mic_mode_toggle(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    
    // Check which switch triggered the event
    if (obj == ui_mic_switch1) {
        // SAFETY SEQUENCE: 
        // 1. Reset Volume to 0 (Prevent Pop)
        AppData.mic_volume = 0;
        
        // 2. Update UI Slider immediately
        if (ui_Slider1) lv_slider_set_value(ui_Slider1, 0, LV_ANIM_ON);
        
        // 3. Send Update (Volume = 0, Relay = OLD)
        AppData.sendUpdate();
        
        // 4. Wait for receiver to process volume drop
        vTaskDelay(pdMS_TO_TICKS(50));
        
        // 5. Toggle Relay
        AppData.mic_relay_state = !AppData.mic_relay_state;
        
        // 6. Sync UI (Updates Button State & Confirms Slider)
        AppData.syncUI();
        
        // 7. Send Update (Relay = NEW)
        AppData.sendUpdate();
    }
    else if (obj == ui_music_switch) {
        // SAFETY SEQUENCE:
        // 1. Reset Volume to 0
        AppData.music_volume = 0;
        
        // 2. Update UI Slider
        if (ui_Slider2) lv_slider_set_value(ui_Slider2, 0, LV_ANIM_ON);
        
        // 3. Send Update (Volume = 0, Relay = OLD)
        AppData.sendUpdate();
        
        // 4. Wait
        vTaskDelay(pdMS_TO_TICKS(50));
        
        // 5. Toggle Relay
        AppData.music_relay_state = !AppData.music_relay_state;
        
        // 6. Sync UI
        AppData.syncUI();
        
        // 7. Send Update (Relay = NEW)
        AppData.sendUpdate();
    }
}

} // extern "C"
