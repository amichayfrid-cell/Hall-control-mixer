#include <Arduino.h>
#include <lvgl.h>
#include <ArduinoJson.h>
#include "ui/ui.h"
#include "bsp.h"
#include "app_data.h"

// Defined in ui_events_impl.cpp
extern void ui_screen2_add_power_toggle(void);

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
    ui_screen2_add_power_toggle();  // Add power sensing toggle to Screen 2
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
        // When charger disconnects, wait 15 seconds before shutting down
        // so the user can navigate to Screen 2 and disable if needed
        if (AppData.power_sensing_enabled) {
            int input_reg = bsp_get_input_state();
            
            static bool system_was_on = true;
            static unsigned long shutdown_timer_start = 0;
            static bool shutdown_pending = false;
            
            if (input_reg != -1) {
                bool switch_is_on = (input_reg & 0x01);
                
                if (switch_is_on) {
                    // Power ON detected
                    if (shutdown_pending) {
                        Serial.println("Power: Charger reconnected, cancelling shutdown.");
                        shutdown_pending = false;
                    }
                    if (!system_was_on) {
                        Serial.println("Power: Switch ON detected. Waking up.");
                        bsp_set_backlight(true);
                        
                        bsp_lvgl_lock(-1);
                        _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Screen3_screen_init);
                        bsp_lvgl_unlock();
                        
                        system_was_on = true;
                        AppData.sendUpdate();
                    }
                } else {
                    // Power OFF detected
                    if (system_was_on && !shutdown_pending) {
                        // Start 15-second countdown
                        shutdown_pending = true;
                        shutdown_timer_start = millis();
                        Serial.println("Power: Charger disconnected. Shutting down in 15 seconds...");
                    }
                    
                    if (shutdown_pending && (millis() - shutdown_timer_start > 15000)) {
                        // 15 seconds passed — execute shutdown
                        Serial.println("Power: Shutdown timer expired. Turning off.");
                        bsp_set_backlight(false);
                        AppData.music_relay_state = false;
                        AppData.sendUpdate();
                        
                        // Send shutdown command to second controller
                        StaticJsonDocument<64> shutdownDoc;
                        shutdownDoc["pwr"] = 0;
                        serializeJson(shutdownDoc, Serial1);
                        Serial1.println();
                        Serial.println("RS485 TX: {\"pwr\":0}");
                        
                        system_was_on = false;
                        shutdown_pending = false;
                    }
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
