#ifndef __LCD_ST7789_H
#define __LCD_ST7789_H

#include <stdint.h>
#include "board.h"

// LCD dimensions - horizontal orientation
#define LCD_W 320
#define LCD_H 240

// LCD MADCTL register value for rotation
#define LCD_MADCTL 0x60

// Common colors (RGB565)
#define LCD_WHITE         0xFFFF
#define LCD_BLACK         0x0000
#define LCD_BLUE          0x001F
#define LCD_RED           0xF800
#define LCD_GREEN         0x07E0
#define LCD_CYAN          0x7FFF
#define LCD_MAGENTA       0xF81F
#define LCD_YELLOW        0xFFE0

// LCD control pins - from board.h
#define LCD_RES_HIGH()    GPIO_SetHigh(GPIO4, GPIO_BIT5)
#define LCD_RES_LOW()     GPIO_SetLow(GPIO4, GPIO_BIT5)
#define LCD_DC_HIGH()     GPIO_SetHigh(GPIO4, GPIO_BIT6)
#define LCD_DC_LOW()      GPIO_SetLow(GPIO4, GPIO_BIT6)
#define LCD_CS_HIGH()     GPIO_SetHigh(GPIO5, GPIO_BIT0)
#define LCD_CS_LOW()      GPIO_SetLow(GPIO5, GPIO_BIT0)
#define LCD_BLK_HIGH()    GPIO_SetHigh(GPIO5, GPIO_BIT1)
#define LCD_BLK_LOW()     GPIO_SetLow(GPIO5, GPIO_BIT1)

// Basic functions
void LCD_Init(void);
void LCD_WriteReg(uint8_t reg);
void LCD_WriteData8(uint8_t data);
void LCD_WriteData16(uint16_t data);
void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

// Drawing functions
void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);

// Streaming functions (DMA with ping-pong buffer)
void LCD_StreamOpen(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_StreamClose(void);
void LCD_StreamSend(const uint8_t *bytes, uint32_t nbytes);
void LCD_StreamFillSolid(uint32_t npix, uint16_t color);

#endif
