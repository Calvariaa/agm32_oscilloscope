#ifndef _IPS200_H_
#define _IPS200_H_

#include <stdint.h>
#include "board.h"

// ---------- pin mapping (same physical pins as lcd_st7789) ----------
#define IPS200_RES_HIGH()   GPIO_SetHigh(GPIO4, GPIO_BIT5)
#define IPS200_RES_LOW()    GPIO_SetLow(GPIO4, GPIO_BIT5)
#define IPS200_DC_HIGH()    GPIO_SetHigh(GPIO4, GPIO_BIT6)
#define IPS200_DC_LOW()     GPIO_SetLow(GPIO4, GPIO_BIT6)
#define IPS200_BLK_HIGH()   GPIO_SetHigh(GPIO5, GPIO_BIT1)
#define IPS200_BLK_LOW()    GPIO_SetLow(GPIO5, GPIO_BIT1)

// ---------- common colors (RGB565) ----------
typedef enum {
    RGB565_WHITE    = 0xFFFF,
    RGB565_BLACK    = 0x0000,
    RGB565_BLUE     = 0x001F,
    RGB565_PURPLE   = 0xF81F,
    RGB565_PINK     = 0xFE19,
    RGB565_RED      = 0xF800,
    RGB565_MAGENTA  = 0xF81F,
    RGB565_GREEN    = 0x07E0,
    RGB565_CYAN     = 0x07FF,
    RGB565_YELLOW   = 0xFFE0,
    RGB565_BROWN    = 0xBC40,
    RGB565_GRAY     = 0x8430,
} rgb565_color_enum;

// ---------- display orientation ----------
typedef enum {
    IPS200_PORTAIT        = 0,
    IPS200_PORTAIT_180    = 1,
    IPS200_CROSSWISE      = 2,
    IPS200_CROSSWISE_180  = 3,
} ips200_dir_enum;

// ---------- font size ----------
typedef enum {
    IPS200_6X8_FONT  = 0,
    IPS200_8X16_FONT = 1,
} ips200_font_size_enum;

// ---------- default settings ----------
#define IPS200_DEFAULT_DISPLAY_DIR   IPS200_CROSSWISE_180
#define IPS200_DEFAULT_PENCOLOR      RGB565_RED
#define IPS200_DEFAULT_BGCOLOR       RGB565_WHITE
#define IPS200_DEFAULT_DISPLAY_FONT  IPS200_8X16_FONT

extern uint16_t ips200_width_max;
extern uint16_t ips200_height_max;

// ---------- public API ----------
void ips200_init        (void);
void ips200_clear       (void);
void ips200_full        (uint16_t color);
void ips200_set_dir     (ips200_dir_enum dir);
void ips200_set_font    (ips200_font_size_enum font);
void ips200_set_color   (uint16_t pen, uint16_t bgcolor);

void ips200_draw_point  (uint16_t x, uint16_t y, uint16_t color);
void ips200_draw_line   (uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, uint16_t color);

void ips200_show_char   (uint16_t x, uint16_t y, char dat);
void ips200_show_string (uint16_t x, uint16_t y, const char dat[]);
void ips200_show_int    (uint16_t x, uint16_t y, int32_t dat, uint8_t num);
void ips200_show_uint   (uint16_t x, uint16_t y, uint32_t dat, uint8_t num);
void ips200_show_float  (uint16_t x, uint16_t y, double dat, uint8_t num, uint8_t pointnum);

void ips200_show_binary_image (uint16_t x, uint16_t y, const uint8_t *image,
                               uint16_t width, uint16_t height,
                               uint16_t dis_width, uint16_t dis_height);
void ips200_show_gray_image   (uint16_t x, uint16_t y, const uint8_t *image,
                               uint16_t width, uint16_t height,
                               uint16_t dis_width, uint16_t dis_height, uint8_t threshold);
void ips200_show_rgb565_image (uint16_t x, uint16_t y, const uint16_t *image,
                               uint16_t width, uint16_t height,
                               uint16_t dis_width, uint16_t dis_height, uint8_t color_mode);
void ips200_show_wave         (uint16_t x, uint16_t y, const uint16_t *wave,
                               uint16_t width, uint16_t value_max,
                               uint16_t dis_width, uint16_t dis_value_max);

#endif
