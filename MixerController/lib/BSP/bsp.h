#pragma once

#include <Arduino.h>

// Board dimensions (also used by app code)
#define TOUCH_WIDTH  800
#define TOUCH_HEIGHT 480

// ---- LVGL Anti-Tearing Configuration ----
// Mode 3: Double buffer + Direct mode (recommended by Waveshare)
#define LVGL_PORT_AVOID_TEARING_MODE (3)

// Derived from mode (do not change)
#define LVGL_PORT_AVOID_TEAR       (1)
#define LVGL_PORT_DISP_BUFFER_NUM  (2)
#define LVGL_PORT_DIRECT_MODE      (1)
#define LVGL_PORT_ROTATION_DEGREE  (0)

// ---- Function Prototypes ----
void bsp_init();           // Initialize board: IO Expander, LCD, Touch, LVGL
bool bsp_lvgl_lock(int timeout_ms = -1);
void bsp_lvgl_unlock();
void bsp_set_backlight(bool on);
int  bsp_get_input_state();
