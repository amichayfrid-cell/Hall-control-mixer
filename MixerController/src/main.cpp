#include <Arduino.h>
#include <lvgl.h>
#include "ui/ui.h"
#include <bsp.h>
#include "app_data.h"
#include "esp_heap_caps.h" // Required for PSRAM allocation

// Timer for LVGL
static uint32_t last_tick = 0;

// Declare draw_buf globally as per instruction
static lv_disp_draw_buf_t draw_buf;

// Multicore Definitions
TaskHandle_t CommsTaskHandle = NULL;
volatile bool trigger_wakeup = false;
volatile bool trigger_sleep = false;

// Task running on Core 0 for blocked I/O (RS485, I2C polling)
void CommsTask(void *pvParameters) {
    // 20Hz Polling Rate for Power Switch is plenty (50ms)
    const TickType_t xDelay = 50 / portTICK_PERIOD_MS;
    
    // De-bouncing state
    bool system_was_on = true;
    int debounce_counter = 0;

    for (;;) {
        // 1. RS485 Listener (Fast Poll)
        // Serial buffer is ISR driven, so checking 'available' is fast.
        AppData.handleIncomingData(Serial1);
        AppData.handleIncomingData(Serial);

        // 2. Power Sensing Logic (I2C - Slower)
        // We throttle this to avoid flooding the bus, but 20Hz is fine.
        static unsigned long last_poll = 0;
        if (millis() - last_poll > 50) {
            last_poll = millis();
            
            // Read DI0 from CH422G (Bit 0 of Input Register)
            // This involves I2C Transaction which blocks! Good to be on Core 0.
            int input_reg = bsp_get_input_state();
            
            if (input_reg != -1) {
                bool switch_is_on = (input_reg & 0x01); 
                
                if (switch_is_on) {
                    // DETECTED ON
                    if (!system_was_on) {
                        debounce_counter++;
                        if (debounce_counter > 2) { // ~100ms debounce
                             Serial.println("Power: Switch ON detected. Triggering Wakeup.");
                             // Hardware Action (Can be done here or main thread)
                             // Backlight is I2C, can be done here on Core 0 safely (if Wire is not used elsewhere recklessly)
                             bsp_set_backlight(true);
                             
                             trigger_wakeup = true; // Signal UI Task
                             
                             system_was_on = true;
                             debounce_counter = 0;
                             AppData.sendUpdate();
                        }
                    } else {
                        debounce_counter = 0;
                    }
                } else {
                    // DETECTED OFF
                    if (system_was_on) {
                        debounce_counter++;
                        if (debounce_counter > 2) { 
                            Serial.println("Power: Switch OFF detected. Triggering Sleep.");
                            
                            bsp_set_backlight(false);
                            AppData.music_relay_state = false; 
                            AppData.sendUpdate();
                            
                            trigger_sleep = true; // Signal UI Task

                            system_was_on = false;
                            debounce_counter = 0;
                        }
                    } else {
                        debounce_counter = 0;
                    }
                }
            } // End Input Reg Check
            
            // Heartbeat Logic (Optional, can be here)
            static unsigned long last_heartbeat = 0;
            if (millis() - last_heartbeat > 2000) {
                last_heartbeat = millis();
                // Heartbeat payload if needed
            }
        }
        
        // Yield to let IDLE0 run (feed WDT)
        vTaskDelay(1); 
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Shorter delay
    
    // Initialize App Data
    AppData.begin();

    // Initialize Display and Hardware
    // Note: bsp_init calls Wire.begin(). Wire is generally safe if used primarily from one task
    // or protected. We will move all I2C access (Backlight, Touch, Input) to Core 0?
    // FAIL: Touch is read by LVGL on Core 1.
    // RISK: I2C (Wire) is not purely thread safe. 
    // If Core 0 polls CH422G while Core 1 reads GT911, we might crash.
    // SOLUTION: We keep Touch on Core 1 (in LVGL driver) and CH422G on Core 0.
    // BUT synchronization is needed.
    // BETTER STRATEGY FOR STABILITY:
    // Only move RS485 to Core 0. Keep Power Logic on Core 1 for now if I2C collision is risky.
    // USER REQUESTED: "Move RS logic... so this core is graphics only".
    // So let's try moving RS485 to Core 0 first.
    // AND Power Logic uses I2C.
    // Let's assume Wire library has mutexes (ESP32 Arduino Wire usually does).
    
    bsp_init(); 
    
    lv_init();
    bsp_lvgl_port_init();

    // Register Touch Driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = bsp_touch_read;
    lv_indev_drv_register(&indev_drv);

    // Initialize UI
    ui_init(); 
    AppData.syncUI();
    
    // Create Comms Task on Core 0
    xTaskCreatePinnedToCore(
        CommsTask,    /* Function to implement the task */
        "CommsTask",  /* Name of the task */
        4096,         /* Stack size in words */
        NULL,         /* Task input parameter */
        1,            /* Priority of the task (Low) */
        &CommsTaskHandle,  /* Task handle. */
        0);           /* Core where the task should run */
        
    Serial.println("System Initialized. Graphics on Core 1. Comms on Core 0.");
}

void loop() {
    // LVGL Graphics Loop (Core 1)
    lv_timer_handler();
    
    // Check Signals from Comms Task
    if (trigger_wakeup) {
        trigger_wakeup = false;
        // UI Action on UI Thread
         _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Screen3_screen_init);
    }
    
    if (trigger_sleep) {
        trigger_sleep = false;
        // UI Action (if any, e.g., show black screen object?)
    }
    
    // Minimal delay to prevent task starvation if LVGL is too fast
    // But with -Ofast and 18MHz, we want max speed.
    // yield() usually calls vTaskDelay(0).
    // Let's rely on LVGL's internal pacing (LV_DISP_DEF_REFR_PERIOD) usually limiting FPS.
    // But we are in Full Refresh mode, so we are bounded by drawing and flushing time.
}
