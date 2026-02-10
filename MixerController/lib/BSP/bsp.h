#pragma once

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <Wire.h>
#include "TAMC_GT911.h"
#include "ui/ui.h"

// Waveshare ESP32-S3-Touch-LCD-4.3B Configuration
// Converted to ESP32_Display_Panel Library

// RGB Interface Pins
#define LCD_DE      5
#define LCD_VSYNC   3
#define LCD_HSYNC   46
#define LCD_PCLK    7

#define LCD_R0      1
#define LCD_R1      2
#define LCD_R2      42
#define LCD_R3      41
#define LCD_R4      40

#define LCD_G0      39
#define LCD_G1      0
#define LCD_G2      45
#define LCD_G3      48
#define LCD_G4      47
#define LCD_G5      21

#define LCD_B0      14
#define LCD_B1      38
#define LCD_B2      18
#define LCD_B3      17
#define LCD_B4      10

// Touch & I2C Conf
#define TOUCH_SDA   8
#define TOUCH_SCL   9
#define TOUCH_INT   4
#define TOUCH_RST   -1
#define TOUCH_WIDTH  800
#define TOUCH_HEIGHT 480

// Display Timing (From Waveshare Example)
#define LCD_TIMING_FREQ_HZ      (18 * 1000 * 1000)
#define LCD_TIMING_HPW          (4)
#define LCD_TIMING_HBP          (8)
#define LCD_TIMING_HFP          (8)
#define LCD_TIMING_VPW          (4)
#define LCD_TIMING_VBP          (16)
#define LCD_TIMING_VFP          (16)
#define LCD_BOUNCE_BUFFER_SIZE  (TOUCH_WIDTH * 20) 

// Manufacturer Demo "Secret Sauce" Macros
#define LVGL_PORT_AVOID_TEAR       (1)
#define LVGL_PORT_AVOID_TEARING_MODE (3) // Mode 3: Double Buffer + Direct Mode
#define LVGL_PORT_DISP_BUFFER_NUM  (2)
#define LVGL_PORT_DIRECT_MODE      (1)
#define LVGL_PORT_ROTATION_DEGREE  (0)
#define LV_INV_BUF_SIZE            (32) // Needed for dirty area tracking structure

// Function Prototypes
void bsp_init();
void bsp_init_display();
void bsp_lvgl_port_init();
void bsp_display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void bsp_touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void *bsp_get_frame_buffer(uint8_t index);
int bsp_get_input_state();
void bsp_set_backlight(bool on);
