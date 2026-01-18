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
    // Sends full state every 2 seconds. 
    // This serves two purposes:
    // 1. Syncs Mixer Body immediately on startup.
    // 2. Acts as "Heartbeat". If Mixer Body stops receiving this, it knows system is OFF.
    static unsigned long last_heartbeat = 0;
    if (millis() - last_heartbeat > 2000) {
        last_heartbeat = millis();
        AppData.sendUpdate();
    }
    
    delay(5);
}
