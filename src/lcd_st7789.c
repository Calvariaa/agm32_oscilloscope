#include "lcd_st7789.h"
#include "board.h"
#include "spi.h"
#include "dmac.h"
#include "gpio.h"
#include <string.h>

static void delay_ms_local(uint32_t ms)
{
    uint32_t start = UTIL_GetTick();
    while ((UTIL_GetTick() - start) < ms);
}

// AGM32 SPI sends low byte first, so for 1-byte: data is just the byte value
// For multi-byte: bytes are packed little-endian into uint32_t
static void spi_send_byte(uint8_t b)
{
    SPI_SetPhaseCtrl(SPI1, SPI_PHASE_0, SPI_PHASE_ACTION_TX, SPI_PHASE_MODE_SINGLE, 1);
    SPI_SetPhaseData(SPI1, SPI_PHASE_0, (uint32_t)b);
    SPI_Start(SPI1, SPI_CTRL_PHASE_CNT1, SPI_CTRL_DMA_OFF, SPI_INTERRUPT_OFF);
    SPI_WaitForDone(SPI1);
}

// Send 2 bytes MSB first (big-endian for ST7789)
static void spi_send_word(uint16_t w)
{
    // AGM32 sends low byte first, so swap: put MSB in low byte position
    uint32_t data = ((uint32_t)(w & 0xFF) << 8) | (w >> 8);
    SPI_SetPhaseCtrl(SPI1, SPI_PHASE_0, SPI_PHASE_ACTION_TX, SPI_PHASE_MODE_SINGLE, 2);
    SPI_SetPhaseData(SPI1, SPI_PHASE_0, data);
    SPI_Start(SPI1, SPI_CTRL_PHASE_CNT1, SPI_CTRL_DMA_OFF, SPI_INTERRUPT_OFF);
    SPI_WaitForDone(SPI1);
}

void LCD_WriteData8(uint8_t data)
{
    LCD_CS_LOW();
    spi_send_byte(data);
    LCD_CS_HIGH();
}

void LCD_WriteData16(uint16_t data)
{
    LCD_CS_LOW();
    spi_send_word(data);
    LCD_CS_HIGH();
}

void LCD_WriteReg(uint8_t reg)
{
    LCD_DC_LOW();
    LCD_CS_LOW();
    spi_send_byte(reg);
    LCD_DC_HIGH();
    LCD_CS_HIGH();
}


void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    LCD_WriteReg(0x2A);  // Column address set
    LCD_WriteData16(x1);
    LCD_WriteData16(x2);
    LCD_WriteReg(0x2B);  // Row address set
    LCD_WriteData16(y1);
    LCD_WriteData16(y2);
    LCD_WriteReg(0x2C);  // Memory write
}

void LCD_Init(void)
{
    // Hardware reset
    LCD_RES_LOW();
    delay_ms_local(100);
    LCD_RES_HIGH();
    delay_ms_local(100);

    // Backlight on
    LCD_BLK_HIGH();
    delay_ms_local(100);

    // ST7789 init sequence
    LCD_WriteReg(0x11);  // SLPOUT - sleep out
    delay_ms_local(120);

    LCD_WriteReg(0x36);  // MADCTL - memory data access control
    LCD_WriteData8(LCD_MADCTL);
    LCD_WriteReg(0x3A);  // COLMOD - interface pixel format
    LCD_WriteData8(0x05);  // 16-bit/pixel

    LCD_WriteReg(0xB2);  // PORCTRL - porch setting
    LCD_WriteData8(0x0C);
    LCD_WriteData8(0x0C);
    LCD_WriteData8(0x00);
    LCD_WriteData8(0x33);
    LCD_WriteData8(0x33);

    LCD_WriteReg(0xB7);  // GCTRL - gate control
    LCD_WriteData8(0x35);

    LCD_WriteReg(0xBB);  // VCOMS - VCOM setting
    LCD_WriteData8(0x19);

    LCD_WriteReg(0xC0);  // LCMCTRL - LCM control
    LCD_WriteData8(0x2C);

    LCD_WriteReg(0xC2);  // VDVVRHEN - VDV and VRH command enable
    LCD_WriteData8(0x01);

    LCD_WriteReg(0xC3);  // VRHS - VRH set
    LCD_WriteData8(0x12);

    LCD_WriteReg(0xC4);  // VDVS - VDV set
    LCD_WriteData8(0x20);

    LCD_WriteReg(0xC6);  // FRCTRL2 - frame rate control
    LCD_WriteData8(0x0F);

    LCD_WriteReg(0xD0);  // PWCTRL1 - power control 1
    LCD_WriteData8(0xA4);
    LCD_WriteData8(0xA1);

    LCD_WriteReg(0xE0);  // PVGAMCTRL - positive voltage gamma
    LCD_WriteData8(0xD0);
    LCD_WriteData8(0x04);
    LCD_WriteData8(0x0D);
    LCD_WriteData8(0x11);
    LCD_WriteData8(0x13);
    LCD_WriteData8(0x2B);
    LCD_WriteData8(0x3F);
    LCD_WriteData8(0x54);
    LCD_WriteData8(0x4C);
    LCD_WriteData8(0x18);
    LCD_WriteData8(0x0D);
    LCD_WriteData8(0x0B);
    LCD_WriteData8(0x1F);
    LCD_WriteData8(0x23);

    LCD_WriteReg(0xE1);  // NVGAMCTRL - negative voltage gamma
    LCD_WriteData8(0xD0);
    LCD_WriteData8(0x04);
    LCD_WriteData8(0x0C);
    LCD_WriteData8(0x11);
    LCD_WriteData8(0x13);
    LCD_WriteData8(0x2C);
    LCD_WriteData8(0x3F);
    LCD_WriteData8(0x44);
    LCD_WriteData8(0x51);
    LCD_WriteData8(0x2F);
    LCD_WriteData8(0x1F);
    LCD_WriteData8(0x1F);
    LCD_WriteData8(0x20);
    LCD_WriteData8(0x23);

    LCD_WriteReg(0x21);  // INVON - display inversion on
    LCD_WriteReg(0x29);  // DISPON - display on
}

void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
{
    LCD_SetAddress(xsta, ysta, xend - 1, yend - 1);
    uint32_t npix = ((uint32_t)(xend - xsta)) * ((uint32_t)(yend - ysta));
    for (uint32_t i = 0; i < npix; i++) {
        LCD_WriteData16(color);
    }
}

void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    LCD_SetAddress(x, y, x, y);
    LCD_WriteData16(color);
}

// Streaming functions (simplified without DMA for now)
void LCD_StreamOpen(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    LCD_DC_LOW();
    LCD_CS_LOW();
    spi_send_byte(0x2A);  // CASET
    LCD_DC_HIGH();
    spi_send_word(x1);
    spi_send_word(x2);

    LCD_DC_LOW();
    spi_send_byte(0x2B);  // RASET
    LCD_DC_HIGH();
    spi_send_word(y1);
    spi_send_word(y2);

    LCD_DC_LOW();
    spi_send_byte(0x2C);  // RAMWR
    LCD_DC_HIGH();
    // CS stays low, DC stays high: pixel data follows
}

void LCD_StreamClose(void)
{
    LCD_CS_HIGH();
}

void LCD_StreamSend(const uint8_t *bytes, uint32_t nbytes)
{
    for (uint32_t i = 0; i < nbytes; i++) {
        spi_send_byte(bytes[i]);
    }
}

void LCD_StreamFillSolid(uint32_t npix, uint16_t color)
{
    for (uint32_t i = 0; i < npix; i++) {
        spi_send_word(color);
    }
}
