#include "ui_oscilloscope.h"
#include "simple_font.h"
#include <string.h>
#include <stdio.h>

static int16_t old_wave[WAVE_WIDTH];
static uint8_t col_bytes[WAVE_HEIGHT * 2];

void ui_init_static_screen(void)
{
	uint16_t i = 0;

	// Clear screen
	LCD_Fill(0, 0, LCD_W, LCD_H, LCD_BLACK);

	// Title
	LCD_Fill(0, 0, LCD_W, 20, LCD_GREEN);
	LCD_DrawLine(0, 20, 0, 197, LCD_GREEN);
	LCD_DrawLine(0, 197, 319, 197, LCD_GREEN);

	// Vertical grid (0.5V/div, 22px per div)
	for(i = 20; i <= 196; i += GRID_SPACING)
	{
		LCD_StreamOpen(1, i, 319, i);
		LCD_StreamFillSolid(319, GRID_COLOR);
		LCD_StreamClose();
	}

	// Horizontal grid
	for(i = 32; i < 319; i += GRID_X_SPACING)
	{
		LCD_StreamOpen(i, 20, i, 196);
		LCD_StreamFillSolid(177, GRID_COLOR);
		LCD_StreamClose();
	}

	// Bottom status bar
	LCD_Fill(0, 198, 319, 239, 0x780F);  // PURPLE
}

void ui_update_dynamic_info(float peak_voltage, float frequency, uint8_t is_running)
{
	// Update status area
	LCD_Fill(0, 210, 60, 239, 0x780F);
	if(is_running)
	{
		// Draw a simple indicator (filled square)
		LCD_Fill(10, 215, 30, 235, LCD_WHITE);
	}

	// Update frequency area - display frequency value
	LCD_Fill(65, 210, 160, 239, 0x780F);
	if(frequency >= 1000)
	{
		font_draw_number(70, 212, frequency / 1000.0f, 1, 3, LCD_WHITE, 0x780F);
	}
	else
	{
		font_draw_number(70, 212, frequency, 0, 3, LCD_WHITE, 0x780F);
	}

	// Update peak voltage area - display voltage value
	LCD_Fill(190, 210, 319, 239, 0x780F);
	font_draw_number(195, 212, peak_voltage, 2, 3, LCD_WHITE, 0x780F);
}

void ui_clear_waveform_area(void)
{
	LCD_Fill(1, 21, 318, 196, LCD_BLACK);
	memset(old_wave, 0, sizeof(old_wave));
}

void ui_draw_waveform(const int16_t *wave_data, uint16_t num_points)
{
	uint16_t i = 0;
	int16_t new_wave[WAVE_WIDTH];

	if(num_points > WAVE_WIDTH)
		num_points = WAVE_WIDTH;

	// Convert ADC data (0-4095) to screen coordinates (20-196)
	// wave_data is in range 0-4095, with 2048 as center (0V)
	// screen y is 20-196, with 196 as center (0V)
	for(i = 0; i < num_points; i++)
	{
		// Normalize ADC value: (value - 2048) / 2048
		// Then scale to pixels: pixel_offset = normalized * 44px/V = normalized * 88 / 2
		float adc_normalized = (wave_data[i] - 2048.0f) / 2048.0f;
		int16_t v = (int16_t)(GRID_Y_0V - adc_normalized * PX_PER_VOLT / 2.0f);
		if(v < 20) v = 20;
		if(v > 196) v = 196;
		new_wave[i] = v;
	}

	// Draw waveform with differential update
	for(i = 0; i < num_points - 1; i++)
	{
		int16_t oa = old_wave[i], ob = old_wave[i + 1];
		int16_t na = new_wave[i], nb = new_wave[i + 1];
		int16_t ot = (oa < ob) ? oa : ob, obt = (oa > ob) ? oa : ob;
		int16_t nt = (na < nb) ? na : nb, nbt = (na > nb) ? na : nb;
		int16_t top = (ot < nt) ? ot : nt, bot = (obt > nbt) ? obt : nbt;
		int16_t y;
		uint32_t bcnt;

		if(top < 20) top = 20;
		if(bot > 196) bot = 196;
		if(bot < top) continue;

		bcnt = 0;
		for(y = top; y <= bot; y++)
		{
			uint16_t gc;
			if((y >= nt) && (y <= nbt))
			{
				gc = LCD_GREEN;
			}
			else if(((GRID_Y_0V - y) % GRID_SPACING == 0) || (i % GRID_X_SPACING == 0))
			{
				gc = (i == 0) ? LCD_GREEN : GRID_COLOR;
			}
			else
			{
				gc = LCD_BLACK;
			}
			col_bytes[bcnt++] = (uint8_t)(gc >> 8);
			col_bytes[bcnt++] = (uint8_t)(gc & 0xFF);
		}

		LCD_StreamOpen(i, top, i, bot);
		LCD_StreamSend(col_bytes, bcnt);
		LCD_StreamClose();
	}

	// Save frame for next iteration
	memcpy(old_wave, new_wave, num_points * sizeof(int16_t));
}
