#include "bsp.h"
#include <Wire.h>
#include <Arduino_GFX_Library.h>

// TAMC_GT911 ts(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);
// We will call begin(addr) later settings
TAMC_GT911 ts(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT); 


// Timing parameters for Waveshare 4.3B (800x480)
// Timing parameters for Waveshare 4.3B (800x480)
// Timing parameters for Waveshare 4.3B (800x480)
// "Stable Rock" Config:
// 1. PCLK 12MHz (Lowest Safe Speed to guarantee NO jitter)
// 2. Official Timings (8/4/8) for correct centering
// 3. Inverted Phase (For signal stability)
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    LCD_DE, LCD_VSYNC, LCD_HSYNC, LCD_PCLK,
    LCD_R0, LCD_R1, LCD_R2, LCD_R3, LCD_R4,
    LCD_G0, LCD_G1, LCD_G2, LCD_G3, LCD_G4, LCD_G5,
    LCD_B0, LCD_B1, LCD_B2, LCD_B3, LCD_B4,
    0, 8, 4, 8,    // HSYNC Polarity 0, HFP=8, HPW=4, HBP=8
    0, 8, 4, 8,    // VSYNC Polarity 0, VFP=8, VPW=4, VBP=8
    1, 12000000, 0, // PCLK 12MHz (Sacrificing FPS for 100% Stability)
    1, 0); // pclk_active_neg = 1

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    TOUCH_WIDTH, TOUCH_HEIGHT, bus);

// Helper to enable backlight via I2C (CH422G)
// CH422G Protocol:
// 1. Enable Output Mode: Write 0x01 to Register 0x24
// 2. Set Pins High: Write 0xFF to Register 0x38 (or 0x26 for input?)
// Note: On Arduino Wire, beginTransmission(addr) takes the 7-bit addr.
// So to write to "Register 0x24", we assume the device responds at 0x24.
// To write to "Register 0x38", we assume it responds at 0x38.
// Helper to reset Touch via CH422G while controlling INT (GPIO 4) to set address
void reset_touch() {
    Serial.println("Performing Hardware Reset on Touch...");
    
    // 1. Prepare INT pin (GPIO 4) to set I2C Address
    // If INT=HIGH during Reset Rising Edge -> Address 0x5D
    // If INT=LOW  during Reset Rising Edge -> Address 0x14
    pinMode(TOUCH_INT, OUTPUT);
    digitalWrite(TOUCH_INT, HIGH); // We aim for 0x5D
    
    // 2. Enable CH422G Output Mode (if not already)
    Wire.beginTransmission(0x24);
    Wire.write(0x01);
    Wire.endTransmission();
    
    // 3. Hold Reset LOW (via CH422G)
    // Writing 0x00 to 0x38 sets all pins LOW (Backlight off, Reset Active)
    Wire.beginTransmission(0x38);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(20);
    
    // 4. Release Reset HIGH (via CH422G)
    // Writing 0xFF to 0x38 sets all pins HIGH (Backlight On, Reset Released)
    Wire.beginTransmission(0x38);
    Wire.write(0xFF);
    Wire.endTransmission();
    delay(10);
    
    // 5. Release INT pin (return to input for driver usage)
    digitalWrite(TOUCH_INT, LOW); // Optional
    pinMode(TOUCH_INT, INPUT);
    delay(200); // Allow GT911 to boot
    
    Serial.println("Touch Reset Complete.");
}

void enable_backlight() {
    // Just ensure outputs are High (Backlight On) without full reset cycle if needed later
    Wire.beginTransmission(0x38);
    Wire.write(0xFF);
    Wire.endTransmission();
}

void bsp_init() {
    Serial.println("BSP Init...");
    
    // Initialize I2C
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    // Wire.setClock(100000); // CH422G works fine at 100k
    
    // --- TOUCH HARDWARE RESET ---
    reset_touch();

    // Debug: Scan again to see if 0x5D or 0x14 appeared
    Serial.println("Scanning I2C for Touch...");
    int touch_addr = 0;
    for (byte i = 0x10; i < 0x60; i++) {
        Wire.beginTransmission(i);
        if (Wire.endTransmission() == 0) {
            if(i == 0x5D || i == 0x14) {
                 Serial.printf("Found Touch at: 0x%02X\n", i);
                 touch_addr = i;
            }
        }
    }

    // Initialize Touch
    if (touch_addr == 0) {
        Serial.println("WARNING: Touch device not found after reset! Defaulting to 0x5D.");
        touch_addr = 0x5D;
    }
    
    ts.begin(touch_addr); 
    ts.setRotation(ROTATION_NORMAL);
    
    // Explicitly read once to clear state
    ts.read();
    Serial.println("Touch Driver Initialized.");

    // Initialize Display
    Serial.println("Init Display...");
    gfx->begin();
    // gfx->fillScreen(RED); // Red screen removed
    
    // Ensure backlight is on again
    enable_backlight();
    
    Serial.println("Display Init Done.");
}

void bsp_display_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *) color_p, w, h);
    lv_disp_flush_ready(disp);
}

void bsp_touch_read(lv_indev_drv_t * indriver, lv_indev_data_t * data) {
    ts.read();
    if (ts.isTouched) {
        data->state = LV_INDEV_STATE_PR;
        // Manual Calibration: Invert Both Axes (180 degree rotation)
        data->point.x = TOUCH_WIDTH - ts.points[0].x;
        data->point.y = TOUCH_HEIGHT - ts.points[0].y;
        
        /*
        Serial.printf("Touch Raw: %d,%d -> Mapped: %d,%d\n", 
            ts.points[0].x, ts.points[0].y, data->point.x, data->point.y);
        */
    } else {
        data->state = LV_INDEV_STATE_REL;
        // Serial.println("Touch: None");
    }
}
