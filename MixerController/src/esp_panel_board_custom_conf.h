/*
 * Board configuration for Waveshare ESP32-S3-Touch-LCD-4.3B (800x480)
 * Based on the official Waveshare demo 09_lvgl_Porting
 */

#pragma once

/**
 * @brief Flag to enable custom board configuration (0/1)
 */
#define ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM  (1)

#if ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// General parameters ///////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_NAME                "Waveshare:ESP32-S3-Touch-LCD-4.3B"

/* LCD resolution in pixels */
#define ESP_PANEL_BOARD_WIDTH         (800)
#define ESP_PANEL_BOARD_HEIGHT        (480)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// LCD panel /////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_USE_LCD             (1)

#if ESP_PANEL_BOARD_USE_LCD

#define ESP_PANEL_BOARD_LCD_CONTROLLER      ST7262
#define ESP_PANEL_BOARD_LCD_BUS_TYPE        (ESP_PANEL_BUS_TYPE_RGB)

#if ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB

    #define ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL       (0)

    /* RGB Timing - from official Waveshare example for 800x480 */
    #define ESP_PANEL_BOARD_LCD_RGB_CLK_HZ            (14 * 1000 * 1000)
    #define ESP_PANEL_BOARD_LCD_RGB_HPW               (4)
    #define ESP_PANEL_BOARD_LCD_RGB_HBP               (8)
    #define ESP_PANEL_BOARD_LCD_RGB_HFP               (8)
    #define ESP_PANEL_BOARD_LCD_RGB_VPW               (4)
    #define ESP_PANEL_BOARD_LCD_RGB_VBP               (8)
    #define ESP_PANEL_BOARD_LCD_RGB_VFP               (8)

    #define ESP_PANEL_BOARD_LCD_RGB_PCLK_ACTIVE_NEG   (1)
    #define ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH         (16)
    #define ESP_PANEL_BOARD_LCD_RGB_PIXEL_BITS         (ESP_PANEL_LCD_COLOR_BITS_RGB565)
    #define ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE   (ESP_PANEL_BOARD_WIDTH * 20)

    /* Pin assignments - from official Waveshare schematic */
    #define ESP_PANEL_BOARD_LCD_RGB_IO_HSYNC        (46)
    #define ESP_PANEL_BOARD_LCD_RGB_IO_VSYNC        (3)
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DE           (5)
    #define ESP_PANEL_BOARD_LCD_RGB_IO_PCLK         (7)
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DISP         (-1)

    /* Data pins D0-D15 (B0-B4, G0-G5, R0-R4) */
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA0        (14)   // B0
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA1        (38)   // B1
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA2        (18)   // B2
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA3        (17)   // B3
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA4        (10)   // B4
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA5        (39)   // G0
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA6        (0)    // G1
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA7        (45)   // G2
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA8        (48)   // G3
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA9        (47)   // G4
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA10       (21)   // G5
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA11       (1)    // R0
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA12       (2)    // R1
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA13       (42)   // R2
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA14       (41)   // R3
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA15       (40)   // R4

#endif // ESP_PANEL_BOARD_LCD_BUS_TYPE

/* Color configuration */
#define ESP_PANEL_BOARD_LCD_COLOR_BITS          (ESP_PANEL_LCD_COLOR_BITS_RGB565)
#define ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER     (0)
#define ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT    (0)

/* Transformation */
#define ESP_PANEL_BOARD_LCD_SWAP_XY             (0)
#define ESP_PANEL_BOARD_LCD_MIRROR_X            (0)
#define ESP_PANEL_BOARD_LCD_MIRROR_Y            (0)
#define ESP_PANEL_BOARD_LCD_GAP_X               (0)
#define ESP_PANEL_BOARD_LCD_GAP_Y               (0)

/* Reset pin */
#define ESP_PANEL_BOARD_LCD_RST_IO              (-1)
#define ESP_PANEL_BOARD_LCD_RST_LEVEL           (0)

#endif // ESP_PANEL_BOARD_USE_LCD

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Touch panel ///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_USE_TOUCH               (1)

#if ESP_PANEL_BOARD_USE_TOUCH

#define ESP_PANEL_BOARD_TOUCH_CONTROLLER        GT911
#define ESP_PANEL_BOARD_TOUCH_BUS_TYPE          (ESP_PANEL_BUS_TYPE_I2C)

#if (ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C) || \
    (ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI)
#define ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST        (0)
#endif

#if ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C
    #define ESP_PANEL_BOARD_TOUCH_I2C_HOST_ID           (0)
#if !ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
    #define ESP_PANEL_BOARD_TOUCH_I2C_CLK_HZ            (400 * 1000)
    #define ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP        (1)
    #define ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP        (1)
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL            (9)
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA            (8)
#endif
    #define ESP_PANEL_BOARD_TOUCH_I2C_ADDRESS           (0)
#endif // ESP_PANEL_BOARD_TOUCH_BUS_TYPE

/* Touch transformation */
#define ESP_PANEL_BOARD_TOUCH_SWAP_XY           (0)
#define ESP_PANEL_BOARD_TOUCH_MIRROR_X          (0)
#define ESP_PANEL_BOARD_TOUCH_MIRROR_Y          (0)

/* Touch control pins */
#define ESP_PANEL_BOARD_TOUCH_RST_IO            (-1)    // Handled by IO expander
#define ESP_PANEL_BOARD_TOUCH_RST_LEVEL         (0)
#define ESP_PANEL_BOARD_TOUCH_INT_IO            (4)
#define ESP_PANEL_BOARD_TOUCH_INT_LEVEL         (0)

#endif // ESP_PANEL_BOARD_USE_TOUCH

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Backlight ////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_USE_BACKLIGHT           (1)

#if ESP_PANEL_BOARD_USE_BACKLIGHT

#define ESP_PANEL_BOARD_BACKLIGHT_TYPE          (ESP_PANEL_BACKLIGHT_TYPE_SWITCH_EXPANDER)
#define ESP_PANEL_BOARD_BACKLIGHT_IO            (2)     // IO Expander pin 2
#define ESP_PANEL_BOARD_BACKLIGHT_ON_LEVEL      (1)
#define ESP_PANEL_BOARD_BACKLIGHT_IDLE_OFF      (0)

#endif // ESP_PANEL_BOARD_USE_BACKLIGHT

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// IO Expander //////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_USE_EXPANDER            (1)

#if ESP_PANEL_BOARD_USE_EXPANDER

#define ESP_PANEL_BOARD_EXPANDER_CHIP           CH422G
#define ESP_PANEL_BOARD_EXPANDER_SKIP_INIT_HOST     (0)
#define ESP_PANEL_BOARD_EXPANDER_I2C_HOST_ID        (0)
#if !ESP_PANEL_BOARD_EXPANDER_SKIP_INIT_HOST
#define ESP_PANEL_BOARD_EXPANDER_I2C_CLK_HZ         (400 * 1000)
#define ESP_PANEL_BOARD_EXPANDER_I2C_SCL_PULLUP     (1)
#define ESP_PANEL_BOARD_EXPANDER_I2C_SDA_PULLUP     (1)
#define ESP_PANEL_BOARD_EXPANDER_I2C_IO_SCL         (9)
#define ESP_PANEL_BOARD_EXPANDER_I2C_IO_SDA         (8)
#endif
#define ESP_PANEL_BOARD_EXPANDER_I2C_ADDRESS        (0x20)

#endif // ESP_PANEL_BOARD_USE_EXPANDER

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Additional initialization code /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Enable all IO expander outputs after begin */
#define ESP_PANEL_BOARD_EXPANDER_POST_BEGIN_FUNCTION(p) \
    {  \
        auto board = static_cast<Board *>(p);  \
        auto expander = static_cast<esp_expander::CH422G*>(board->getIO_Expander()->getBase()); \
        expander->enableAllIO_Output(); \
        return true;    \
    }

/* Reset LCD via IO Expander before LCD begin */
#define ESP_PANEL_BOARD_LCD_PRE_BEGIN_FUNCTION(p) \
    {  \
        constexpr int LCD_RST = 3; \
        auto board = static_cast<Board *>(p);  \
        auto expander = board->getIO_Expander()->getBase(); \
        expander->digitalWrite(LCD_RST, 0); \
        vTaskDelay(pdMS_TO_TICKS(10)); \
        expander->digitalWrite(LCD_RST, 1); \
        vTaskDelay(pdMS_TO_TICKS(100)); \
        return true;    \
    }

/* Reset Touch via IO Expander before Touch begin */
#define ESP_PANEL_BOARD_TOUCH_PRE_BEGIN_FUNCTION(p) \
    {  \
        constexpr gpio_num_t TP_INT = static_cast<gpio_num_t>(ESP_PANEL_BOARD_TOUCH_INT_IO); \
        constexpr int TP_RST = 1; \
        auto board = static_cast<Board *>(p);  \
        auto expander = board->getIO_Expander()->getBase(); \
        gpio_set_direction(TP_INT, GPIO_MODE_OUTPUT); \
        gpio_set_level(TP_INT, 0); \
        vTaskDelay(pdMS_TO_TICKS(10)); \
        expander->digitalWrite(TP_RST, 0); \
        vTaskDelay(pdMS_TO_TICKS(100)); \
        expander->digitalWrite(TP_RST, 1); \
        vTaskDelay(pdMS_TO_TICKS(200)); \
        gpio_reset_pin(TP_INT); \
        return true;    \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// File Version ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR 1
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MINOR 0
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_PATCH 0

#endif // ESP_PANEL_BOARD_USE_CUSTOM
