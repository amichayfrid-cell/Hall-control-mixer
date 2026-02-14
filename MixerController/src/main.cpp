#include <Arduino.h>
#include <lvgl.h>
#include "ui/ui.h"
#include "bsp.h"
#include "app_data.h"

void setup() {
    Serial.begin(115200);
    delay(2000);  // Wait for USB CDC
    
    Serial.println("\n\n=== MixerController Starting ===");
    Serial.printf("PSRAM: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());

    // 1. Initialize App Data (Preferences, RS485)
    AppData.begin();

    // 2. Initialize BSP (IO Expander, LCD, Touch, LVGL)
    //    This also starts the LVGL task on Core 1
    bsp_init();

    // 3. Initialize UI (must be done inside LVGL mutex)
    bsp_lvgl_lock(-1);
    ui_init();
    AppData.syncUI();
    bsp_lvgl_unlock();

    Serial.println("=== Setup Complete ===");
    Serial.printf("Free Heap after init: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
    // RS485 Listener
    AppData.handleIncomingData(Serial1);
    
    // USB Serial Listener (For Testing)
    AppData.handleIncomingData(Serial);
    
    // Heartbeat & Power Sensing (every 2 seconds)
    static unsigned long last_heartbeat = 0;
    if (millis() - last_heartbeat > 2000) {
        last_heartbeat = millis();
        
        // --- POWER SENSING LOGIC (USB CHARGER on DI0) ---
        int input_reg = bsp_get_input_state();
        
        static bool system_was_on = true;
        static int debounce_counter = 0;
        
        if (input_reg != -1) {
            bool switch_is_on = (input_reg & 0x01);
            
            if (switch_is_on) {
                if (!system_was_on) {
                    debounce_counter++;
                    if (debounce_counter > 0) {
                        Serial.println("Power: Switch ON detected. Waking up.");
                        bsp_set_backlight(true);
                        
                        // Show loading screen, then transition to main screen
                        bsp_lvgl_lock(-1);
                        _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Screen3_screen_init);
                        bsp_lvgl_unlock();
                        
                        system_was_on = true;
                        debounce_counter = 0;
                        AppData.sendUpdate();
                    }
                } else {
                    debounce_counter = 0;
                }
            } else {
                if (system_was_on) {
                    debounce_counter++;
                    if (debounce_counter > 0) {
                        Serial.println("Power: Switch OFF detected. Shutting down.");
                        bsp_set_backlight(false);
                        AppData.music_relay_state = false;
                        AppData.sendUpdate();
                        system_was_on = false;
                        debounce_counter = 0;
                    }
                } else {
                    debounce_counter = 0;
                }
            }
        }
        
        // Regular Heartbeat
        AppData.sendUpdate();
    }
    
    // Throttled NVS save — only when dirty, max once per 10 seconds
    static unsigned long last_save = 0;
    if (AppData.dirty && (millis() - last_save > 10000)) {
        AppData.saveState();
        AppData.dirty = false;
        last_save = millis();
    }
    
    // Small yield - LVGL runs in its own task, so loop just handles I/O
    vTaskDelay(pdMS_TO_TICKS(10));
}
