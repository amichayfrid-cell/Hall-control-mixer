#include "bsp.h"
#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include <Wire.h>
#include "TAMC_GT911.h"

using namespace esp_panel::drivers;

LCD *lcd = nullptr;
Touch *touch = nullptr;
bool is_lcd_ready = false;

// Touch & I2C Objects
TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);

// CH422G Register "Addresses" (Effective I2C Addresses)
#define CH422G_REG_SET 0x24 
#define CH422G_REG_IO  0x38 

// =============================================================================
// Manufacturer Demo Logic (Ported from lvgl_v8_port.cpp)
// =============================================================================

// Helper for pixel copying
__attribute__((always_inline))
static inline void copy_pixel_16bpp(uint8_t *to, const uint8_t *from)
{
    *(uint16_t *)to++ = *(const uint16_t *)from++;
}

#define COPY_PIXEL(_bpp, to, from)  copy_pixel_16bpp(to, from)

// Rotation/Copy Logic (Simplified for 0 degree)
__attribute__((always_inline))
IRAM_ATTR static inline void rotate_copy_pixel(
    const uint8_t *from, uint8_t *to, uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, uint16_t w,
    uint16_t h, uint16_t rotate
)
{
    int from_bytes_per_piexl = sizeof(lv_color_t);
    int from_bytes_per_line = w * from_bytes_per_piexl;
    int from_index = 0;

    int to_bytes_per_piexl = 16 >> 3; // RGB565
    int to_bytes_per_line = w * to_bytes_per_piexl;
    int to_index_const = (h - 1) * to_bytes_per_line + (w - x_start - 1) * to_bytes_per_piexl;
    int to_index = 0;

    // Direct Copy (0 Degree)
    int from_index_const = x_start * from_bytes_per_piexl;
    to_index_const = x_start * to_bytes_per_line;
    for (int from_y = y_start; from_y < y_end + 1; from_y++) {
        from_index = from_y * from_bytes_per_line + from_index_const;
        to_index = to_index_const + from_y * to_bytes_per_line;
        for (int from_x = x_start; from_x < x_end + 1; from_x++) {
             COPY_PIXEL(16, to + to_index, from + from_index);
             from_index += from_bytes_per_piexl;
             to_index += to_bytes_per_piexl;
        }
    }
}

// Dirty Area Tracking for Direct Mode
typedef struct {
    uint16_t inv_p;
    uint8_t inv_area_joined[LV_INV_BUF_SIZE];
    lv_area_t inv_areas[LV_INV_BUF_SIZE];
} lv_port_dirty_area_t;

static lv_port_dirty_area_t dirty_area;

static void flush_dirty_save(lv_port_dirty_area_t *dirty_area)
{
    lv_disp_t *disp = _lv_refr_get_disp_refreshing();
    dirty_area->inv_p = disp->inv_p;
    for (int i = 0; i < disp->inv_p; i++) {
        dirty_area->inv_area_joined[i] = disp->inv_area_joined[i];
        dirty_area->inv_areas[i] = disp->inv_areas[i];
    }
}

typedef enum {
    FLUSH_STATUS_PART,
    FLUSH_STATUS_FULL
} lv_port_flush_status_t;

typedef enum {
    FLUSH_PROBE_PART_COPY,
    FLUSH_PROBE_SKIP_COPY,
    FLUSH_PROBE_FULL_COPY,
} lv_port_flush_probe_t;

static lv_port_flush_probe_t flush_copy_probe(lv_disp_drv_t *drv)
{
    static lv_port_flush_status_t prev_status = FLUSH_STATUS_PART;
    lv_port_flush_status_t cur_status;
    lv_port_flush_probe_t probe_result;
    lv_disp_t *disp_refr = _lv_refr_get_disp_refreshing();

    uint32_t flush_ver = 0;
    uint32_t flush_hor = 0;
    for (int i = 0; i < disp_refr->inv_p; i++) {
        if (disp_refr->inv_area_joined[i] == 0) {
            flush_ver = (disp_refr->inv_areas[i].y2 + 1 - disp_refr->inv_areas[i].y1);
            flush_hor = (disp_refr->inv_areas[i].x2 + 1 - disp_refr->inv_areas[i].x1);
            break;
        }
    }
    cur_status = ((flush_ver == drv->ver_res) && (flush_hor == drv->hor_res)) ? (FLUSH_STATUS_FULL) : (FLUSH_STATUS_PART);

    if (prev_status == FLUSH_STATUS_FULL) {
        if ((cur_status == FLUSH_STATUS_PART)) {
            probe_result = FLUSH_PROBE_FULL_COPY;
        } else {
            probe_result = FLUSH_PROBE_SKIP_COPY;
        }
    } else {
        probe_result = FLUSH_PROBE_PART_COPY;
    }
    prev_status = cur_status;

    return probe_result;
}

static void *get_next_frame_buffer(LCD *lcd)
{
    static void *next_fb = NULL;
    static void *fbs[2] = { NULL };

    if (next_fb == NULL) {
        fbs[0] = lcd->getFrameBufferByIndex(0);
        fbs[1] = lcd->getFrameBufferByIndex(1);
        next_fb = fbs[1];
    } else {
        next_fb = (next_fb == fbs[0]) ? fbs[1] : fbs[0];
    }

    return next_fb;
}

static inline void *flush_get_next_buf(LCD *lcd)
{
    return get_next_frame_buffer(lcd);
}

static void flush_dirty_copy(void *dst, void *src, lv_port_dirty_area_t *dirty_area)
{
    lv_coord_t x_start, x_end, y_start, y_end;
    for (int i = 0; i < dirty_area->inv_p; i++) {
        if (dirty_area->inv_area_joined[i] == 0) {
            x_start = dirty_area->inv_areas[i].x1;
            x_end = dirty_area->inv_areas[i].x2;
            y_start = dirty_area->inv_areas[i].y1;
            y_end = dirty_area->inv_areas[i].y2;

            rotate_copy_pixel(
                (uint8_t *)src, (uint8_t *)dst, x_start, y_start, x_end, y_end, TOUCH_WIDTH, TOUCH_HEIGHT, 0
            );
        }
    }
}

// SIMPLIFIED FLUSH CALLBACK (Stability First)
static void bsp_flush_callback(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    // In Direct Mode + Full Refresh:
    // color_map points to the framebuffer we just drew into.
    // We simply check if it's the last part of the flush (always is for full refresh)
    // and switch the LCD to display this buffer.
    
    if (lv_disp_flush_is_last(drv)) {
        lcd->switchFrameBufferTo(color_map);
    }

    lv_disp_flush_ready(drv);
}

// =============================================================================
// BSP Init
// =============================================================================

void bsp_init_display() {
    BusRGB::RefreshPanelPartialConfig refresh_config;
    
    refresh_config.pclk_gpio_num = LCD_PCLK;
    refresh_config.hsync_gpio_num = LCD_HSYNC;
    refresh_config.vsync_gpio_num = LCD_VSYNC;
    refresh_config.de_gpio_num = LCD_DE;
    refresh_config.disp_gpio_num = -1;
    
    refresh_config.data_gpio_nums[0] = LCD_B0;
    refresh_config.data_gpio_nums[1] = LCD_B1;
    refresh_config.data_gpio_nums[2] = LCD_B2;
    refresh_config.data_gpio_nums[3] = LCD_B3;
    refresh_config.data_gpio_nums[4] = LCD_B4;
    
    refresh_config.data_gpio_nums[5] = LCD_G0;
    refresh_config.data_gpio_nums[6] = LCD_G1;
    refresh_config.data_gpio_nums[7] = LCD_G2;
    refresh_config.data_gpio_nums[8] = LCD_G3;
    refresh_config.data_gpio_nums[9] = LCD_G4;
    refresh_config.data_gpio_nums[10] = LCD_G5;
    
    refresh_config.data_gpio_nums[11] = LCD_R0;
    refresh_config.data_gpio_nums[12] = LCD_R1;
    refresh_config.data_gpio_nums[13] = LCD_R2;
    refresh_config.data_gpio_nums[14] = LCD_R3;
    refresh_config.data_gpio_nums[15] = LCD_R4;

    // Timing
    refresh_config.pclk_hz = LCD_TIMING_FREQ_HZ;
    refresh_config.h_res = TOUCH_WIDTH;
    refresh_config.v_res = TOUCH_HEIGHT;
    refresh_config.hsync_pulse_width = LCD_TIMING_HPW;
    refresh_config.hsync_back_porch = LCD_TIMING_HBP;
    refresh_config.hsync_front_porch = LCD_TIMING_HFP;
    refresh_config.vsync_pulse_width = LCD_TIMING_VPW;
    refresh_config.vsync_back_porch = LCD_TIMING_VBP;
    refresh_config.vsync_front_porch = LCD_TIMING_VFP;
    
    refresh_config.flags_pclk_active_neg = true; 
    refresh_config.bounce_buffer_size_px = LCD_BOUNCE_BUFFER_SIZE;

    // Create Main Bus Config and assign the partial config
    BusRGB::Config bus_config;
    bus_config.refresh_panel = refresh_config;

    // LCD Config
    LCD::DevicePartialConfig device_config;
    device_config.bits_per_pixel = 16;
    
    LCD::Config lcd_config;
    lcd_config.device = device_config;

    // Create Driver
    lcd = new LCD_ST7262(bus_config, lcd_config);
    lcd->configFrameBufferNumber(2);

    if (!lcd->init()) { Serial.println("LCD Init Fail"); return; }
    if (!lcd->begin()) { Serial.println("LCD Begin Fail"); return; }
    
    gpio_set_drive_capability((gpio_num_t)LCD_PCLK, GPIO_DRIVE_CAP_3);
    lcd->setDisplayOnOff(true);
    is_lcd_ready = true;
}

void bsp_lvgl_port_init() {
    static lv_disp_draw_buf_t draw_buf;
    void *buf1 = lcd->getFrameBufferByIndex(0);
    void *buf2 = lcd->getFrameBufferByIndex(1);
    
    lv_disp_draw_buf_init(&draw_buf, (lv_color_t*)buf1, (lv_color_t*)buf2, TOUCH_WIDTH * TOUCH_HEIGHT);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    
    disp_drv.hor_res = TOUCH_WIDTH;
    disp_drv.ver_res = TOUCH_HEIGHT;
    disp_drv.flush_cb = bsp_flush_callback; 
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = (void *)lcd;
    // Simplified Mode: Direct + Full Refresh (Ping Pong)
    disp_drv.direct_mode = 1;
    disp_drv.full_refresh = 1;
    
    lv_disp_drv_register(&disp_drv);
}

// CH422G I2C Helper
void ch422g_write(uint8_t addr_cmd, uint8_t data) {
    Wire.beginTransmission(addr_cmd);
    Wire.write(data);
    Wire.endTransmission();
}

void bsp_set_backlight(bool on) {
    // CH422G IO 2 Control
    // To set Output, we write to REG_IO (0x38).
    // We need to preserve other bits?
    // Since we don't track state, we'll just assume "ON" means setting Bit 2.
    // And "Reset" pins (1 and 3) should be High (Inactive) during operations.
    // So 0xFF is safe. 
    // If OFF: Clear Bit 2? -> 0xFB.
    // However, during init we set everything to 0xFF.
    // So for ON, we just write 0xFF (Simple).
    if (on) {
        ch422g_write(CH422G_REG_IO, 0xFF);
    } else {
        ch422g_write(CH422G_REG_IO, 0xFB); // Clear Bit 2
    }
}

int bsp_get_input_state() {
    // Crude implementation
    return 1;
}

void bsp_touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    ts.read();
    if (ts.isTouched) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = ts.points[0].x;
        data->point.y = ts.points[0].y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// Global Init Wrapper
void bsp_init() {
    // 1. I2C Init for Touch and Expander
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    delay(10);

    // 2. Init IO Expander (CH422G)
    // Enable Output Mode (REG_SET = 0x01) via Address 0x24
    ch422g_write(CH422G_REG_SET, 0x01);
    
    // 3. Reset Sequence for LCD and Touch
    // We need to toggle LCD_RST (IO3) and TP_RST (IO1).
    // Backlight (IO2) should be ON (High) ideally, or we can enable it later.
    // Let's hold Reset (Low) for 10ms.
    // Pattern: IO3=0, IO1=0. IO2=1 (BL ON). Result: 0xF5 (1111 0101)
    ch422g_write(CH422G_REG_IO, 0xF5); 
    delay(20);
    
    // Release Reset (High). All High = 0xFF.
    ch422g_write(CH422G_REG_IO, 0xFF);
    delay(200);

    // 4. Init Touch (Now that it's out of reset)
    ts.begin();
    ts.setRotation(ROTATION_INVERTED);

    // 5. Init Display
    bsp_init_display();
}

void *bsp_get_frame_buffer(uint8_t index) {
    if (lcd && is_lcd_ready) return lcd->getFrameBufferByIndex(index);
    return nullptr;
}

void bsp_display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    lv_disp_flush_ready(disp);
}
