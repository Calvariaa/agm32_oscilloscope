#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "alta.h"

#ifndef CLOCK_PERIOD
#define CLOCK_PERIOD (1e9/BOARD_PLL_FREQUENCY)
#endif

#ifndef RTC_PERIOD
#define RTC_PERIOD (1e9f/BOARD_RTC_FREQUENCY)
#endif

// Blue Pill LED on PIN_2 -> GPIO0_0
#define LED_GPIO      GPIO0
#define LED_GPIO_MASK APB_MASK_GPIO0
#define LED_GPIO_BIT  GPIO_BIT0

#define MIN_IRQ_PRIORITY 1
#define MAX_IRQ_PRIORITY PLIC_MAX_PRIORITY

#define I2C_PRIORITY    (MIN_IRQ_PRIORITY + 1)
#define TIMER_PRIORITY  (MIN_IRQ_PRIORITY + 2)
#define DMAC_PRIORITY   (MIN_IRQ_PRIORITY + 8)
#define UART_PRIORITY   (MIN_IRQ_PRIORITY + 9)
#define CAN_PRIORITY    (MIN_IRQ_PRIORITY + 7)
#define RTC_PRIORITY    (MIN_IRQ_PRIORITY + 6)
#define EXT_PRIORITY    (MIN_IRQ_PRIORITY + 4)
#define SPI_PRIORITY    (MIN_IRQ_PRIORITY + 5)
#define MEMSPI_PRIORITY (MIN_IRQ_PRIORITY + 1)
#define GPIO_PRIORITY   (MIN_IRQ_PRIORITY + 1)
#define FLASH_PRIORITY  (MAX_IRQ_PRIORITY - 5)
#define USB_PRIORITY    (MAX_IRQ_PRIORITY - 1)
#define MAC_PRIORITY    (MAX_IRQ_PRIORITY - 1)
#define WDOG_PRIORITY   (MAX_IRQ_PRIORITY - 0)

// Board initialization functions
SYS_HSE_BypassTypeDef board_hse_source(void);
RTC_ClkSourceTypeDef  board_rtc_source(void);
uint32_t board_lse_freq(void);
uint32_t board_pll_clkin_freq(void);

void board_init(void);

static inline void HardFault_Handler(void) { asm("ebreak"); }

#ifdef __cplusplus
}
#endif

#endif
