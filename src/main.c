#include "main.h"
#include "lcd_st7789.h"

static void delay_ms(uint32_t ms)
{
  uint32_t start = UTIL_GetTick();
  while ((UTIL_GetTick() - start) < ms);
}

int main(void)
{
  board_init();

  INT_SetIRQThreshold(MIN_IRQ_PRIORITY);

  // Initialize LCD
  LCD_Init();

  // Test display - fill screen with colors
  LCD_Fill(0, 0, LCD_W, LCD_H, LCD_RED);
  delay_ms(1000);
  LCD_Fill(0, 0, LCD_W, LCD_H, LCD_GREEN);
  delay_ms(1000);
  LCD_Fill(0, 0, LCD_W, LCD_H, LCD_BLUE);

  while (1) {
    GPIO_Toggle(LED_GPIO, LED_GPIO_BIT);
    delay_ms(500);
  }
}
