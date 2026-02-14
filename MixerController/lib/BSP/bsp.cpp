/*
 * BSP Implementation for Waveshare ESP32-S3-Touch-LCD-4.3B
 * Uses ESP32_Display_Panel library (same as official Waveshare demos)
 *
 * Key architecture:
 * - Board object handles LCD, Touch, IO Expander, Backlight
 * - LVGL runs in its own FreeRTOS task with mutex protection
 * - Anti-tearing mode 3: double buffer + direct mode
 * - Bounce buffer for PSRAM bandwidth optimization
 */

#include "bsp.h"
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "esp_timer.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// ---- Constants ----
#define LVGL_PORT_TICK_PERIOD_MS        (2)
#define LVGL_PORT_TASK_MAX_DELAY_MS     (500)
#define LVGL_PORT_TASK_MIN_DELAY_MS     (2)
#define LVGL_PORT_TASK_STACK_SIZE       (6 * 1024)
#define LVGL_PORT_TASK_PRIORITY         (2)
#define LVGL_PORT_BUFFER_NUM_MAX        (2)

// ---- Static globals ----
static Board *board = nullptr;
static SemaphoreHandle_t lvgl_mux = nullptr;
static TaskHandle_t lvgl_task_handle = nullptr;
static void *lvgl_buf[LVGL_PORT_BUFFER_NUM_MAX] = {};

// ======================================================================
// LVGL Flush Callback (Anti-tearing Mode 3: Direct Mode + Double Buffer)
// ======================================================================

static void flush_callback(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    LCD *lcd = (LCD *)drv->user_data;

    /* Action after last area refresh */
    if (lv_disp_flush_is_last(drv)) {
        /* Switch the current LCD frame buffer to `color_map` */
        lcd->switchFrameBufferTo(color_map);

        /* Waiting for the last frame buffer to complete transmission */
        ulTaskNotifyValueClear(NULL, ULONG_MAX);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }

    lv_disp_flush_ready(drv);
}

// ======================================================================
// VSync Callback (notifies LVGL task that frame transmission is done)
// ======================================================================

IRAM_ATTR bool onLcdVsyncCallback(void *user_data)
{
    BaseType_t need_yield = pdFALSE;
    TaskHandle_t task_handle = (TaskHandle_t)user_data;
    xTaskNotifyFromISR(task_handle, ULONG_MAX, eNoAction, &need_yield);
    return (need_yield == pdTRUE);
}

// ======================================================================
// Rounder callback (ensures coordinates are aligned)
// ======================================================================

static void rounder_callback(lv_disp_drv_t *drv, lv_area_t *area)
{
    LCD *lcd = (LCD *)drv->user_data;
    uint8_t x_align = lcd->getBasicAttributes().basic_bus_spec.x_coord_align;
    uint8_t y_align = lcd->getBasicAttributes().basic_bus_spec.y_coord_align;

    if (x_align > 1) {
        area->x1 &= ~(x_align - 1);
        area->x2 = (area->x2 & ~(x_align - 1)) + x_align - 1;
    }
    if (y_align > 1) {
        area->y1 &= ~(y_align - 1);
        area->y2 = (area->y2 & ~(y_align - 1)) + y_align - 1;
    }
}

// ======================================================================
// LVGL Display Init
// ======================================================================

static lv_disp_t *display_init(LCD *lcd)
{
    if (!lcd || !lcd->getRefreshPanelHandle()) {
        Serial.println("BSP: ERROR - LCD not initialized!");
        return nullptr;
    }

    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv;

    auto lcd_width = lcd->getFrameWidth();
    auto lcd_height = lcd->getFrameHeight();

    // Anti-tearing: use LCD frame buffers directly (in PSRAM, managed by driver)
    int buffer_size = lcd_width * lcd_height;
    for (int i = 0; i < LVGL_PORT_DISP_BUFFER_NUM && i < LVGL_PORT_BUFFER_NUM_MAX; i++) {
        lvgl_buf[i] = lcd->getFrameBufferByIndex(i);
    }

    lv_disp_draw_buf_init(&disp_buf, lvgl_buf[0], lvgl_buf[1], buffer_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = flush_callback;
    disp_drv.hor_res = lcd_width;
    disp_drv.ver_res = lcd_height;
    disp_drv.direct_mode = 1;      // Anti-tearing mode 3
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = (void *)lcd;

    // Rounder callback for coordinate alignment
    if ((lcd->getBasicAttributes().basic_bus_spec.x_coord_align > 1) ||
        (lcd->getBasicAttributes().basic_bus_spec.y_coord_align > 1)) {
        disp_drv.rounder_cb = rounder_callback;
    }

    return lv_disp_drv_register(&disp_drv);
}

// ======================================================================
// LVGL Touch Init
// ======================================================================

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    Touch *tp = (Touch *)indev_drv->user_data;
    TouchPoint point;

    int read_result = tp->readPoints(&point, 1, 0);
    if (read_result > 0) {
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static lv_indev_t *indev_init(Touch *tp)
{
    if (!tp || !tp->getPanelHandle()) {
        Serial.println("BSP: Touch not available, skipping input init");
        return nullptr;
    }

    static lv_indev_drv_t indev_drv_tp;
    lv_indev_drv_init(&indev_drv_tp);
    indev_drv_tp.type = LV_INDEV_TYPE_POINTER;
    indev_drv_tp.read_cb = touchpad_read;
    indev_drv_tp.user_data = (void *)tp;

    return lv_indev_drv_register(&indev_drv_tp);
}

// ======================================================================
// LVGL Task (runs on Core 1)
// ======================================================================

static void lvgl_port_task(void *arg)
{
    Serial.println("BSP: LVGL task started");
    uint32_t task_delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;

    while (1) {
        if (bsp_lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            bsp_lvgl_unlock();
        }
        if (task_delay_ms > LVGL_PORT_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_PORT_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_PORT_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

// ======================================================================
// Public API
// ======================================================================

void bsp_init()
{
    Serial.println("BSP: Initializing board...");

    // 1. Create and init Board (handles IO Expander, LCD, Touch, Backlight)
    board = new Board();
    board->init();

    // 2. Configure LCD for anti-tearing
    auto lcd = board->getLCD();
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);

    // 3. Configure bounce buffer (critical for ESP32-S3 + PSRAM)
    auto lcd_bus = lcd->getBus();
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 20);
        Serial.printf("BSP: Bounce buffer set to %d pixels\n", lcd->getFrameWidth() * 20);
    }

    // 4. Begin board (starts all drivers)
    if (!board->begin()) {
        Serial.println("BSP: ERROR - board->begin() FAILED!");
        return;
    }
    Serial.println("BSP: Board started successfully");

    // 5. Initialize LVGL
    lv_init();
    Serial.println("BSP: LVGL initialized");

    // 6. Setup display driver
    lv_disp_t *disp = display_init(lcd);
    if (!disp) {
        Serial.println("BSP: ERROR - display_init failed!");
        return;
    }
    lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);

    // 7. Setup touch input
    Touch *tp = board->getTouch();
    if (tp) {
        indev_init(tp);
        Serial.println("BSP: Touch initialized");
    }

    // 8. Create LVGL mutex
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    if (!lvgl_mux) {
        Serial.println("BSP: ERROR - failed to create LVGL mutex!");
        return;
    }

    // 9. Create LVGL task on Core 1
    BaseType_t ret = xTaskCreatePinnedToCore(
        lvgl_port_task, "lvgl", LVGL_PORT_TASK_STACK_SIZE, NULL,
        LVGL_PORT_TASK_PRIORITY, &lvgl_task_handle, 1  // Core 1
    );
    if (ret != pdPASS) {
        Serial.println("BSP: ERROR - failed to create LVGL task!");
        return;
    }

    // 10. Attach VSync callback for anti-tearing synchronization
    lcd->attachRefreshFinishCallback(onLcdVsyncCallback, (void *)lvgl_task_handle);

    Serial.println("BSP: Init complete! Display + Touch + LVGL running.");
}

bool bsp_lvgl_lock(int timeout_ms)
{
    if (!lvgl_mux) return false;
    const TickType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE);
}

void bsp_lvgl_unlock()
{
    if (lvgl_mux) {
        xSemaphoreGiveRecursive(lvgl_mux);
    }
}

void bsp_set_backlight(bool on)
{
    if (board) {
        auto backlight = board->getBacklight();
        if (backlight) {
            if (on) {
                backlight->on();
            } else {
                backlight->off();
            }
            Serial.printf("BSP: Backlight %s\n", on ? "ON" : "OFF");
        }
    }
}

int bsp_get_input_state()
{
    if (!board) return -1;
    auto expander = board->getIO_Expander();
    if (!expander) return -1;
    auto base = expander->getBase();
    if (!base) return -1;
    
    // Read DI0 (digital input 0) - used for USB power sensing
    int val = base->digitalRead(0);
    return val;
}
