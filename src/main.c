#include "main.h"
#include "ips200.h"
#include "util.h"
#include <math.h>

#define SINE_W  320
#define SINE_H  240
#define SINE_AMP  (SINE_H / 2 - 1)
#define SINE_MID  (SINE_H / 2)
#define PI 3.14159265f

int main(void)
{
  board_init();
  INT_SetIRQThreshold(MIN_IRQ_PRIORITY);
  ips200_init();

  float phase = 0.0f;
  float phase2 = 0.0f;
  uint16_t prev_y[SINE_W];
  uint16_t new_y[SINE_W];

  ips200_full(RGB565_BLACK);

  while (1) {
    phase += 1.f * sinf(phase2);
    phase2 += 0.001f;
    if (phase >= 2.0f * PI) phase -= 2.0f * PI;
    if (phase2 >= 2.0f * PI) phase2 -= 2.0f * PI;

    for (int x = 0; x < SINE_W; x++) {
      float angle = (float)x / SINE_W * 2.0f * PI + phase;
      new_y[x] = (uint16_t)(SINE_MID - SINE_AMP * sinf(angle));
    }

    ips200_update_wave(prev_y, new_y, SINE_W, RGB565_BLACK, RGB565_GREEN);
  }
}