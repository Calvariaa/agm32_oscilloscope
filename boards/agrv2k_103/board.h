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
