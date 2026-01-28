#include <Arduino.h>
#include <lvgl.h>
#include "ui/ui.h"
#include <bsp.h>
#include "app_data.h"

// Timer for LVGL
static uint32_t last_tick = 0;

void setup() {
    Serial.begin(115200);
    // Give time for USB CDC to enumerate
    delay(8000); 
    // Initialize App Data (Preferences, RS485)
    AppData.begin();

    // Initialize Display and Keys
    bsp_init(); // This should init hardware

    // Check if we need to manually init LVGL if BSP didn't fully handle it
    // Usually 'ui_init' calls lv_init. Check ui.c usually.
    // ui_init() inside ui.c initializes LVGL styles and screens. 
    // It DOES NOT initialize the LVGL library itself (lv_init) or drivers.
    
    lv_init();
    
    // Register Display Driver
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[TOUCH_WIDTH * 10]; // Buffer for 10 lines
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TOUCH_WIDTH * 10);

    // Initialize specific display driver here if not in bsp_init structure
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TOUCH_WIDTH;
    disp_drv.ver_res = TOUCH_HEIGHT;
    disp_drv.flush_cb = bsp_display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Register Touch Driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = bsp_touch_read;
    lv_indev_drv_register(&indev_drv);

    // Initialize UI
    Serial.println("Initializing UI...");
    ui_init(); 
    Serial.println("UI Initialized.");
    
    // Sync UI with Saved Data
    AppData.syncUI();
}

void loop() {
    // Heartbeat
    if (millis() - last_tick > 1000) {
        last_tick = millis();
        // Serial.println("Loop running...");
    }

    // LVGL Task Handler
    lv_timer_handler();
    
    // RS485 Listener
    AppData.handleIncomingData(Serial1);
    
    // USB Serial Listener (For Testing)
    AppData.handleIncomingData(Serial);
    
    // Heartbeat & Sync Mechanism
    static unsigned long last_heartbeat = 0;
    if (millis() - last_heartbeat > 2000) {
        last_heartbeat = millis();
        
        // --- POWER SENSING LOGIC (USB CHARGER) ---
        // Read DI0 from CH422G (Bit 0 of Input Register 0x26)
        int input_reg = bsp_get_input_state();
        
        static bool system_was_on = true; // State tracking
        static int debounce_counter = 0;  // Simple debounce
        
        if (input_reg != -1) {
            // Check Bit 0 (DI0) - Active HIGH (5V from USB)
            bool switch_is_on = (input_reg & 0x01); 
            
            if (switch_is_on) {
                // DETECTED ON
                if (!system_was_on) {
                    debounce_counter++;
                    // Faster Wake-Up: > 0 means 1 cycle (approx 2s latency).
                    // This is enough to let speakers click on safely.
                    if (debounce_counter > 0) { 
                         // Turn System ON
                         Serial.println("Power: Switch ON detected. Waking up.");
                         bsp_set_backlight(true);
                         
                         // UX UPGRADE: 
                         // Show "Loading..." Screen (Screen 3) instead of jump to Main.
                         // Screen 3 already has a logic to wait 2s then go to Screen 1.
                         // This visualizes the "Anti-Pop" safety delay perfectly to the user.
                         _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Screen3_screen_init);
                         
                         system_was_on = true;
                         debounce_counter = 0;
                         // Optimization: Trigger immediate update
                         AppData.sendUpdate();
                    }
                } else {
                    debounce_counter = 0;
                }
            } else {
                // DETECTED OFF
                if (system_was_on) {
                    debounce_counter++;
                    if (debounce_counter > 0) { // Fast Off (1 cycle = 2s delay max)
                        Serial.println("Power: Switch OFF detected. Shutting down.");
                        
                        // 1. Turn off Screen
                        bsp_set_backlight(false);
                        
                        // 2. SAFETY: Turn off Bluetooth Relay (Switch to Line-In)
                        AppData.music_relay_state = false; 
                        // AppData.music_volume = 0; // Optional: Volume Reset?
                        
                        // 3. Send Update to Mixer Body immediately
                        AppData.sendUpdate();
                        
                        system_was_on = false;
                        debounce_counter = 0;
                    }
                } else {
                    debounce_counter = 0;
                }
            }
        }
        
        // Regular Heartbeat (Keeps Mixer Body alive)
        // Even if screen is off, we keep sending heartbeat so Mixer Body doesn't timeout
        // and kill Bluetooth on its own (though we just killed it explicitly).
        // Sending updates ensures sync.
        AppData.sendUpdate();
    }
    
    delay(5);
}
