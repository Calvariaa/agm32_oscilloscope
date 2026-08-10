#include "board.h"

__attribute__((weak))
SYS_HSE_BypassTypeDef board_hse_source(void)
{
#ifdef BOARD_HSE_BYPASS
  return BOARD_HSE_BYPASS;
#else
  return SYS_HSE_BYPASS_OFF;
#endif
}

__attribute__((weak))
RTC_ClkSourceTypeDef board_rtc_source(void)
{
#ifdef BOARD_RTC_SOURCE
  return BOARD_RTC_SOURCE;
#else
  return RTC_CLK_SOURCE_LSE;
#endif
}

uint32_t board_lse_freq(void)
{
#ifdef BOARD_LSE_FREQ
  return BOARD_LSE_FREQ;
#else
  return 32768;
#endif
}

uint32_t board_pll_clkin_freq(void)
{
#ifdef BOARD_PLL_CLKIN_FREQ
  return BOARD_PLL_CLKIN_FREQ;
#else
  return SYS_GetHSEFreq();
#endif
}

__attribute__((weak)) uint32_t FCB_GetPLLFreq(uint32_t clkin_freq);
__attribute__((weak)) void board_init(void)
{
  // Initialize DMAC
  PERIPHERAL_ENABLE(DMAC, 0);
  DMAC_Init();

#ifdef AGM_LOGIC_EMBED_INC
  // Load embedded logic configuration
  static __attribute__((section(".text.logic"))) __attribute__((aligned(4))) const uint8_t logic_config[] = {
#include AGM_LOGIC_EMBED_INC
  };
  PERIPHERAL_ENABLE(FCB, 0);
  SYS_SwitchHSIClock();
  if (sizeof(logic_config) < FCB_AUTO_WORDS * 4) {
    FCB_AutoDecompress((uint32_t)logic_config);
  } else {
    FCB_AutoConfigDma((uint32_t)logic_config, DMAC_CHANNEL0);
  }
  PERIPHERAL_DISABLE(FCB, 0);
#endif

  // Configure PLL frequency
  if (FCB_GetPLLFreq) {
    uint32_t freq = BOARD_PLL_FREQUENCY;
    SYS_EnableAPBClock(APB_MASK_FCB0);
    if (FCB_IsActive()) {
      freq = FCB_GetPLLFreq(board_pll_clkin_freq());
    } else {
      FCB_Activate();
    }
    SYS_SetPLLFreq(freq);
  }

  // Switch to PLL clock
  SYS_SwitchPLLClock(board_hse_source());

  // Initialize interrupts
  INT_Init();
  INT_EnableIntGlobal();
  INT_EnableIntExternal();

  // Initialize LED GPIO (PIN_2 -> GPIO0_0)
  SYS_EnableAPBClock(LED_GPIO_MASK);
  GPIO_SetOutput(LED_GPIO, LED_GPIO_BIT);
  GPIO_SetHigh(LED_GPIO, LED_GPIO_BIT); // LED off initially

  // Initialize SPI1 for LCD
  PERIPHERAL_ENABLE_ALL(SPI, 1);
  SYS_EnableAPBClock(APB_MASK_SPI1);
  SPI_Init(SPI1, SPI_CTRL_SCLK_DIV16);  // 200MHz/16 = 12.5MHz, safe for init

  // Initialize LCD control GPIO
  SYS_EnableAPBClock(APB_MASK_GPIO4);
  GPIO_SetOutput(GPIO4, GPIO_BIT6 | GPIO_BIT7);  // RES, DC
  GPIO_SetHigh(GPIO4, GPIO_BIT6 | GPIO_BIT7);

  SYS_EnableAPBClock(APB_MASK_GPIO5);
  GPIO_SetOutput(GPIO5, GPIO_BIT0 | GPIO_BIT1);  // CS, BLK
  GPIO_SetHigh(GPIO5, GPIO_BIT0 | GPIO_BIT1);

#ifdef LOGGER_UART
  // Initialize UART0 for debug output
  GPIO_AF_ENABLE(GPIO_AF_PIN(UART, LOGGER_UART, UARTRXD));
  GPIO_AF_ENABLE(GPIO_AF_PIN(UART, LOGGER_UART, UARTTXD));

  MSG_UART = UARTx(LOGGER_UART);
  SYS_EnableAPBClock(APB_MASK_UARTx(LOGGER_UART));
  UART_Init(UARTx(LOGGER_UART), BAUD_RATE, UART_LCR_DATABITS_8,
            UART_LCR_STOPBITS_1, UART_LCR_PARITY_NONE, UART_LCR_FIFO_16);

  dbg_printf("\nBoard init done. CLK: %.3fMHz\n",
             SYS_GetSysClkFreq()/(double)1e6);
#endif
}

