#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "TAMC_GT911.h"
#include "ui/ui.h"

// Corrected Pin Definitions for Waveshare ESP32-S3-Touch-LCD-4.3B
// Source: Community and Waveshare Wiki (RGB Panel + CH422G)

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
// Note: Many 16-bit (5-6-5) panels map RGB565 bits to specific pins.
// If R0-R2 are LSBs, and we use 16-bit color, we might need to verify which pins carry the MSBs.
// However, the provided mapping is standard for this board.

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

// Backlight (Controlled via CH422G I2C Expander at 0x24)
// This definition is kept for compatibility but not used as a direct GPIO.
#define LCD_BL      -1 

// Function Prototypes
void bsp_init();
void bsp_display_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p);
void bsp_touch_read(lv_indev_drv_t * indriver, lv_indev_data_t * data);
