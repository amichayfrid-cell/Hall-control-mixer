
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>


/*====================
   COLOR SETTINGS
 *====================*/

/*Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888)*/
#define LV_COLOR_DEPTH 16

/*Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface (e.g. SPI)*/
#define LV_COLOR_16_SWAP 0

/*1: Enable screen transparency.
 *Useful for OSD or other overlapping GUIs.
 *Requires `LV_COLOR_SCREEN_TRANSP = 1` in `lv_conf.h` (this file)*/
#define LV_COLOR_SCREEN_TRANSP 0

/*Images pixels with this color will not be drawn if they are chroma keyed)*/
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)         /*pure green*/

/*=========================
   MEMORY SETTINGS
 *=========================*/

/*1: use custom malloc/free, 0: use the built-in `lv_mem_alloc` and `lv_mem_free`*/
#define LV_MEM_CUSTOM 0

/*Size of the memory available for `lv_mem_alloc` in bytes (>= 2kB)*/
#define LV_MEM_SIZE (48U * 1024U)          /*[bytes]*/

/*Set an address for the memory pool instead of allocating it as a normal array. Can be in external SRAM too.*/
#define LV_MEM_ADR 0     /*0: unused*/

/*====================
   HAL SETTINGS
 *====================*/

/*1: use a custom tick source (it removes the need to manually update the tick with `lv_tick_inc`)*/
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"         /*Header for the system time function*/
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())    /*Expression evaluating to current system time in ms*/


/*==============================
   LIBRARIES
 *==============================*/
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0
#define LV_USE_FS_FATFS 0

/*=========================
   TEXT SETTINGS
 *=========================*/

/*Select a character encoding for strings.
 *Your IDE or editor should have the same character encoding
 *- LV_TXT_ENC_UTF8
 *- LV_TXT_ENC_ASCII
 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_USE_BIDI 1
#define LV_USE_ARABIC_PERSIAN_CHARS 1

/*Can break (wrap) texts on these chars*/
#define LV_TXT_BREAK_CHARS " ,.;:-_"


/*==================
 *    FONT USAGE
 *================*/

/*Montserrat fonts with ASCII range and some symbols using bpp = 4
 *https://fonts.google.com/specimen/Montserrat*/
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/*Demonstrate special features*/
#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0  /*bpp = 3*/
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0  /*Hebrew, Arabic, Persian letters and all their forms*/
#define LV_FONT_SIMSUN_16_CJK            0  /*1000 common Chinese characters*/

/*Pixel perfect monospace fonts*/
#define LV_FONT_UNISCII_8  0
#define LV_FONT_UNISCII_16 0

/*Declare the type of the user fonts*/
#define LV_FONT_FMT_TXT_LARGE 0

/*Set the default font*/
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*===================
   DEMO USAGE
 * =================*/

/*Show some widget. It might be required to increase `LV_MEM_SIZE` */
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0

#endif
