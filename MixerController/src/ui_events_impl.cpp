#include "ui/ui.h"
#include "app_data.h"

// Power sensing toggle switch (created dynamically on Screen 2)
static lv_obj_t *ui_power_switch = NULL;

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

// Power sensing toggle callback
static void toggle_power_sensing(lv_event_t * e) {
    lv_obj_t * sw = lv_event_get_target(e);
    AppData.power_sensing_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    AppData.sendUpdate();
    AppData.saveState();  // Save immediately — this is a settings change
    Serial.printf("Power sensing: %s\n", AppData.power_sensing_enabled ? "ON" : "OFF");
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

// Called after ui_Screen2_screen_init to add the power sensing toggle
void ui_screen2_add_power_toggle(void) {
    if (!ui_Screen2) return;
    
    // Container panel for the toggle (bottom-right area)
    lv_obj_t *panel = lv_obj_create(ui_Screen2);
    lv_obj_set_size(panel, 250, 60);
    lv_obj_align(panel, LV_ALIGN_BOTTOM_RIGHT, -20, -15);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x2C2C2E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, 200, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 15, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 8, LV_PART_MAIN);
    
    // Label
    lv_obj_t *label = lv_label_create(panel);
    lv_label_set_text(label, "כיבוי אוטומטי");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &ui_font_Hebrew30, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 5, 0);
    
    // Switch
    ui_power_switch = lv_switch_create(panel);
    lv_obj_set_size(ui_power_switch, 55, 30);
    lv_obj_align(ui_power_switch, LV_ALIGN_RIGHT_MID, -5, 0);
    
    // Set initial state from AppData
    if (AppData.power_sensing_enabled) {
        lv_obj_add_state(ui_power_switch, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(ui_power_switch, LV_STATE_CHECKED);
    }
    
    // Style the switch
    lv_obj_set_style_bg_color(ui_power_switch, lv_color_hex(0x4CD964), LV_PART_INDICATOR | LV_STATE_CHECKED);
    
    // Event
    lv_obj_add_event_cb(ui_power_switch, toggle_power_sensing, LV_EVENT_VALUE_CHANGED, NULL);
}
