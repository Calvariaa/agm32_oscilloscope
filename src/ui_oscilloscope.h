#ifndef __UI_OSCILLOSCOPE_H
#define __UI_OSCILLOSCOPE_H

#include <stdint.h>
#include "lcd_st7789.h"

// Oscilloscope UI configuration
#define GRID_Y_0V        196
#define GRID_SPACING     22
#define GRID_X_SPACING   32
#define PX_PER_VOLT      44.0f
#define GRID_COLOR       0x03E0
#define WAVE_WIDTH       320
#define WAVE_HEIGHT      240

// Waveform state
#define OSC_PAUSE  0x01
#define OSC_RUN    0x02

// UI Functions
void ui_init_static_screen(void);
void ui_update_dynamic_info(float peak_voltage, float frequency, uint8_t is_running);
void ui_draw_waveform(const int16_t *wave_data, uint16_t num_points);
void ui_clear_waveform_area(void);

#endif
