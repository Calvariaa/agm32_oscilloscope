/*
 * @Description:
 * @Author: Calvaria
 * @Date: 2025-01-10 22:09:43
 */
#include "game_tetris.h"
#include "menu.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "key.h"
#include "st7789.h"

static void (*current_func)();
static bool game_quit_flag = false;

typedef struct
{
    char *name;
    void (*func)();
} menu_select_t;

/**
 * @description: 在xy地绘制一个俄罗斯方块，注意区分内外颜色
 * @param {uint16_t} x
 * @param {uint16_t} y
 * @param {uint16_t} color_outer
 * @param {uint16_t} color_inner
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-22 06:54:22
 */
static void draw_colorblock(int16_t x, int16_t y, uint16_t color_outer, uint16_t color_inner)
{
    // 12x12格子
    ST7789_Fill(x, y, x + 11, y + 12, BLACK);
    ST7789_Fill(x + 1, y + 1, x + 10, y + 11, color_outer);
    ST7789_Fill(x + 2, y + 9, x + 9, y + 10, BLACK);
    ST7789_Fill(x + 1, y + 2, x + 8, y + 9, color_inner);
}

/**
 * @description: 把draw_colorblock生成的方块fill为黑色，简单直接
 * @param {uint16_t} x
 * @param {uint16_t} y
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-22 06:57:01
 */
static void erase_colorblock(int16_t x, int16_t y)
{
    ST7789_Fill(x, y, x + 11, y + 12, BLACK);
}

static const uint16_t title_image_data[24][80] = {
    {0x0040, 0x08C1, 0x08C1, 0x08C1, 0x08A1, 0x08E1, 0x0080, 0x08C1, 0x08C1, 0x0080, 0x00A0, 0x08C1, 0x08A1, 0x08A1, 0x08C1, 0x0060, 0x0040, 0x08C1, 0x08C1, 0x0060, 0x08C1, 0x08C1, 0x00A0, 0x08C1, 0x08C1, 0x08C1, 0x08C1, 0x08A1, 0x08C1, 0x0020, 0x0020, 0x08C1, 0x08A1, 0x08C1, 0x00A0, 0x08C1, 0x0080, 0x08C1, 0x08C1, 0x00A0, 0x08A1, 0x08C1, 0x08A1, 0x08A1, 0x08C1, 0x0080, 0x0040, 0x08C1, 0x08C1, 0x00A0, 0x0080, 0x08C1, 0x08A1, 0x08C1, 0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x08A1, 0x08C1, 0x08C1, 0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0921, 0x0921, 0x00A0, 0x08C1, 0x08C1, 0x08C1, 0x08C1, 0x08C1, 0x08C1, 0x0060},
    {0x1A83, 0x4EC9, 0x4688, 0x4668, 0x4F09, 0x2BE5, 0x2364, 0x4EE9, 0x4EE9, 0x2C45, 0x1AA3, 0x4F09, 0x4668, 0x4668, 0x4F29, 0x2324, 0x2304, 0x4F09, 0x4EE9, 0x3486, 0x1202, 0x46A8, 0x46A8, 0x4668, 0x4688, 0x4688, 0x4688, 0x4668, 0x4F09, 0x1202, 0x11E2, 0x4F09, 0x4668, 0x4668, 0x4EE9, 0x34A6, 0x2384, 0x4EE9, 0x46A8, 0x3D67, 0x1AE3, 0x4EC9, 0x4688, 0x4668, 0x4F09, 0x2C45, 0x2324, 0x4F09, 0x46A8, 0x3D87, 0x34C6, 0x4EE9, 0x4668, 0x46A8, 0x3DA7, 0x2C25, 0x0921, 0x0000, 0x0000, 0x08C1, 0x4648, 0x4688, 0x46A8, 0x0961, 0x0000, 0x0000, 0x0000, 0x0000, 0x08E1, 0x34A6, 0x1222, 0x2304, 0x4EE9, 0x4668, 0x4688, 0x4688, 0x4688, 0x4668, 0x4EE9, 0x2C05},
    {0x1AC3, 0x4F69, 0x4F09, 0x4F29, 0x4F29, 0x08C1, 0x2364, 0x578A, 0x4F69, 0x3506, 0x0020, 0x4668, 0x4F49, 0x4EE9, 0x57AA, 0x2364, 0x2344, 0x57AA, 0x4F69, 0x3526, 0x0000, 0x11C2, 0x4EC9, 0x4F69, 0x4EE9, 0x4F09, 0x4F09, 0x4F09, 0x57AA, 0x1222, 0x1202, 0x57AA, 0x4EE9, 0x4F09, 0x4F29, 0x08E1, 0x2364, 0x57AA, 0x4F29, 0x4648, 0x0020, 0x3526, 0x4F69, 0x4EE9, 0x578A, 0x34A6, 0x2364, 0x57AA, 0x4F49, 0x3DE7, 0x3526, 0x4F69, 0x4F09, 0x4F09, 0x4F49, 0x4F69, 0x4F09, 0x11C2, 0x0000, 0x08E1, 0x4EC9, 0x4F29, 0x4F49, 0x1182, 0x0000, 0x0000, 0x0000, 0x0000, 0x3D47, 0x57AA, 0x11C2, 0x0000, 0x2C65, 0x578A, 0x4EE9, 0x4F09, 0x4F09, 0x4F09, 0x4F69, 0x2C65},
    {0x1AA3, 0x4F49, 0x4EC9, 0x4F49, 0x2384, 0x0000, 0x2BC5, 0x4F49, 0x4F29, 0x3526, 0x0000, 0x1A43, 0x4F29, 0x4EC9, 0x578A, 0x2364, 0x2344, 0x576A, 0x4F29, 0x3506, 0x0000, 0x0000, 0x0941, 0x3DE7, 0x4F69, 0x4EC9, 0x4EE9, 0x4EC9, 0x576A, 0x1222, 0x1202, 0x576A, 0x4EA8, 0x4F69, 0x2C65, 0x0000, 0x2BC5, 0x4F69, 0x4F09, 0x4628, 0x0000, 0x1182, 0x4F29, 0x4EC9, 0x4F69, 0x3486, 0x2344, 0x576A, 0x4F09, 0x3DC7, 0x3506, 0x4F49, 0x4EE9, 0x4EE9, 0x4EE9, 0x4EC9, 0x4F49, 0x3D87, 0x0040, 0x08C1, 0x4EC9, 0x4EE9, 0x4F09, 0x1182, 0x0000, 0x0020, 0x0000, 0x1AA3, 0x4F29, 0x4EE9, 0x4F09, 0x0921, 0x0000, 0x3D67, 0x4F49, 0x4EC9, 0x4EE9, 0x4EC9, 0x4F49, 0x2C45},
    {0x1AA3, 0x4F29, 0x4F29, 0x4628, 0x0040, 0x0000, 0x2BA5, 0x5749, 0x4F29, 0x3506, 0x0000, 0x0000, 0x3506, 0x4F49, 0x5769, 0x2364, 0x2344, 0x5769, 0x4F29, 0x3506, 0x0000, 0x0020, 0x0000, 0x0080, 0x3506, 0x578A, 0x4EE9, 0x4EC9, 0x5769, 0x1202, 0x11E2, 0x5769, 0x4F09, 0x4628, 0x00A0, 0x0000, 0x2BA5, 0x5749, 0x4F09, 0x4608, 0x0040, 0x0000, 0x34A6, 0x4F49, 0x5749, 0x3486, 0x2344, 0x5769, 0x4F09, 0x3DC7, 0x3506, 0x5749, 0x4EC9, 0x4EE9, 0x4EE9, 0x4EE9, 0x4EC9, 0x5769, 0x1A43, 0x0060, 0x4EC9, 0x4EE9, 0x4F09, 0x1182, 0x0000, 0x0020, 0x0000, 0x34A6, 0x5749, 0x4EA8, 0x4F49, 0x3D67, 0x0040, 0x08A0, 0x4688, 0x4F29, 0x4EC9, 0x4EC9, 0x5749, 0x2C45},
    {0x1AC3, 0x478A, 0x4749, 0x1A63, 0x0000, 0x0000, 0x23E5, 0x47AA, 0x478A, 0x3547, 0x0000, 0x0000, 0x0961, 0x46C9, 0x4FEA, 0x2384, 0x1B64, 0x4FCA, 0x478A, 0x3547, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2C25, 0x4F49, 0x3F29, 0x47CA, 0x1223, 0x1202, 0x47AA, 0x478A, 0x1AE3, 0x0000, 0x0000, 0x23E5, 0x47AA, 0x4749, 0x3E68, 0x0040, 0x0000, 0x00A0, 0x4688, 0x47EA, 0x2CC6, 0x1B64, 0x4FCA, 0x476A, 0x3E08, 0x3547, 0x47AA, 0x4729, 0x4749, 0x4749, 0x4749, 0x4729, 0x47CA, 0x2C66, 0x0060, 0x4729, 0x4749, 0x476A, 0x0982, 0x0000, 0x0000, 0x0060, 0x3E88, 0x476A, 0x4749, 0x4729, 0x4FCA, 0x2CE6, 0x0020, 0x1222, 0x4EE9, 0x4749, 0x4729, 0x47AA, 0x2C86},
    {0x4121, 0xC323, 0x7A62, 0x0000, 0x0000, 0x0000, 0x5981, 0xBB22, 0xBB02, 0x8222, 0x0000, 0x0000, 0x0000, 0x51E2, 0xCB63, 0x5161, 0x5161, 0xBB23, 0xBB02, 0x8222, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x5A22, 0xC302, 0xBB23, 0x30E0, 0x30C0, 0xC323, 0x92C2, 0x0040, 0x0000, 0x0000, 0x5981, 0xBB23, 0xB302, 0x9AA2, 0x0000, 0x0000, 0x0000, 0x51E2, 0xCB63, 0x71E1, 0x5161, 0xBB23, 0xB302, 0x9262, 0x8222, 0xBB22, 0xB2E2, 0xB2E2, 0xB2E2, 0xB2E2, 0xAAE2, 0xBB23, 0x71C1, 0x0820, 0xAAE2, 0xB2E2, 0xB302, 0x20A0, 0x0000, 0x0000, 0x0820, 0xA2A2, 0xB302, 0xB2E2, 0xB2E2, 0xAAE2, 0xBB23, 0x8120, 0x0000, 0x3161, 0xC323, 0xAAE2, 0xBB22, 0x69C1},
    {0x5860, 0xF900, 0x4000, 0x0000, 0x0000, 0x0000, 0x7860, 0xF0E0, 0xF0E0, 0xA0A0, 0x0000, 0x0000, 0x0000, 0x2000, 0xF0E0, 0x7880, 0x6860, 0xF900, 0xF0E0, 0xA0A0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0800, 0x7040, 0xF900, 0x4840, 0x4040, 0xF900, 0x5000, 0x0000, 0x0000, 0x0000, 0x7860, 0xF100, 0xE8E0, 0xC8C0, 0x0000, 0x0000, 0x0000, 0x1000, 0xD8C0, 0xA8A0, 0x6860, 0xF8E0, 0xE8E0, 0xC0C0, 0xA8A0, 0xF0E0, 0xE0E0, 0xE0E0, 0xE0E0, 0xE0E0, 0xE0E0, 0xF0E0, 0x8080, 0x0800, 0xE0E0, 0xE8E0, 0xE8E0, 0x3020, 0x0000, 0x0000, 0x0800, 0xD0C0, 0xE8E0, 0xE0E0, 0xE0E0, 0xE0E0, 0xE0E0, 0xF140, 0x3860, 0x0000, 0x9860, 0xE8E0, 0xF0E0, 0x9080},
    {0x3860, 0x88E0, 0x0000, 0x0000, 0x0000, 0x0000, 0x70A0, 0xE960, 0xE160, 0xA0E0, 0x0000, 0x0000, 0x0000, 0x0000, 0x58A0, 0x5080, 0x68A0, 0xE960, 0xE160, 0xA0E0, 0x0000, 0x0000, 0x0000, 0x0000, 0x4860, 0x2840, 0x0000, 0x0000, 0x5880, 0x3860, 0x2840, 0x98E0, 0x0000, 0x0000, 0x0000, 0x0000, 0x70A0, 0xE960, 0xD940, 0xC120, 0x0000, 0x0000, 0x0000, 0x0000, 0x5880, 0x68A0, 0x60A0, 0xE960, 0xE140, 0xB900, 0xA0E0, 0xE160, 0xD940, 0xD940, 0xD940, 0xD940, 0xD940, 0xE160, 0x3040, 0x0800, 0xD940, 0xD940, 0xE140, 0x2840, 0x0000, 0x0000, 0x0800, 0xC120, 0xE160, 0xD940, 0xD940, 0xD940, 0xD940, 0xE140, 0xC920, 0x1820, 0x0820, 0xC940, 0xE960, 0x80C0},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x70A0, 0xE940, 0xE140, 0xA0E0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x78A0, 0xE940, 0xE140, 0xA0E0, 0x0000, 0x0000, 0x0000, 0x6080, 0xE940, 0x3040, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x70A0, 0xE940, 0xE140, 0xC100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x70A0, 0xE940, 0xE140, 0xB900, 0xA0E0, 0xE140, 0xD940, 0xD940, 0xD920, 0xD920, 0xF160, 0x90C0, 0x0000, 0x1820, 0xD120, 0xD940, 0xE140, 0x2840, 0x0000, 0x0000, 0x0000, 0x80A0, 0xE940, 0xD920, 0xD940, 0xD940, 0xD940, 0xD940, 0xE940, 0xA0E0, 0x0000, 0x3840, 0xE940, 0x88C0},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x70A0, 0xE940, 0xE140, 0xA0E0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x78A0, 0xE940, 0xE140, 0xA0E0, 0x0000, 0x0000, 0x88C0, 0xE940, 0xE140, 0x3040, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x70A0, 0xE940, 0xE140, 0xC100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x70A0, 0xE940, 0xE140, 0xB900, 0xA8E0, 0xF160, 0xE140, 0xE940, 0xE940, 0xE940, 0xB900, 0x2020, 0x0000, 0x1820, 0xD120, 0xD940, 0xE140, 0x2840, 0x0000, 0x0000, 0x0000, 0x1820, 0xD920, 0xE120, 0xD940, 0xD940, 0xD940, 0xD940, 0xD920, 0xE940, 0x68A0, 0x0000, 0x78A0, 0x98C0},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x70A0, 0xE940, 0xE940, 0xA0E0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x78A0, 0xE940, 0xE940, 0x98C0, 0x1020, 0xA100, 0xE940, 0xD920, 0xE140, 0x3040, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x70A0, 0xE940, 0xE120, 0xC100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x70A0, 0xE940, 0xE140, 0xB900, 0x7880, 0xB100, 0xA8E0, 0xB100, 0x88C0, 0x5060, 0x0000, 0x0000, 0x0000, 0x1820, 0xD920, 0xE120, 0xE140, 0x2840, 0x0000, 0x0000, 0x0000, 0x0000, 0x90C0, 0xE960, 0xD920, 0xD920, 0xD920, 0xD920, 0xD920, 0xD920, 0xE140, 0x3020, 0x0000, 0x4860},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2ACE, 0x5D7A, 0x5D5A, 0x3BB3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2ACE, 0x5D7A, 0x5D7A, 0x3B92, 0x10A3, 0x5CD7, 0x555B, 0x5519, 0x5D5A, 0x1125, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2ACE, 0x5D7A, 0x5D3A, 0x4C97, 0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2AAD, 0x5D7A, 0x553A, 0x4C97, 0x0906, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0883, 0x54F9, 0x553A, 0x5D3A, 0x1105, 0x0000, 0x0000, 0x0062, 0x08C4, 0x0820, 0x5456, 0x5D5A, 0x551A, 0x553A, 0x553A, 0x553A, 0x551A, 0x5D3A, 0x44F9, 0x08E5, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1B51, 0x367F, 0x365F, 0x2478, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1B51, 0x367F, 0x365F, 0x2498, 0x0000, 0x00E5, 0x2D5C, 0x367F, 0x363F, 0x0947, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1B51, 0x369F, 0x363F, 0x2D7D, 0x0021, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1B31, 0x369F, 0x363F, 0x2D3C, 0x2C36, 0x0967, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00A3, 0x35FF, 0x363F, 0x365F, 0x0947, 0x0000, 0x0000, 0x0083, 0x2C77, 0x0021, 0x0126, 0x361F, 0x363F, 0x361F, 0x363F, 0x363F, 0x363F, 0x361F, 0x3E9F, 0x2372, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2330, 0x3E5F, 0x3E3F, 0x2C56, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2331, 0x3E5F, 0x3E3F, 0x2C56, 0x0000, 0x0000, 0x0062, 0x2C36, 0x469F, 0x0946, 0x0000, 0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2330, 0x465F, 0x3DFF, 0x353B, 0x0021, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1B10, 0x465F, 0x3E1F, 0x34FA, 0x2CB8, 0x353B, 0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00A3, 0x3DBE, 0x3DFF, 0x3E1F, 0x0946, 0x0000, 0x0000, 0x0041, 0x3DDF, 0x2393, 0x0000, 0x11EA, 0x3E1F, 0x3DDF, 0x3DFF, 0x3DFF, 0x3DFF, 0x3DFF, 0x3E1F, 0x357D, 0x08E4},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2331, 0x3E5F, 0x3E3F, 0x2C57, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2331, 0x3E5F, 0x3E3F, 0x2C57, 0x0000, 0x0020, 0x0000, 0x0000, 0x2393, 0x0967, 0x0000, 0x0000, 0x0905, 0x0967, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2331, 0x465F, 0x3E1F, 0x355C, 0x0021, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1B10, 0x465F, 0x3E1F, 0x34FA, 0x2C56, 0x469F, 0x2BF5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00A3, 0x3DDE, 0x3DFF, 0x3E1F, 0x0946, 0x0000, 0x0000, 0x0041, 0x357C, 0x3E5F, 0x1A6D, 0x0000, 0x2352, 0x3E7F, 0x3DDF, 0x3DFF, 0x3DFF, 0x3DFF, 0x3DFF, 0x3E5F, 0x1B10},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1B31, 0x363F, 0x361F, 0x2457, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1B31, 0x363F, 0x361F, 0x2457, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0041, 0x0000, 0x0148, 0x35FE, 0x126D, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1B31, 0x365F, 0x35FF, 0x2D3C, 0x0021, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1B10, 0x365F, 0x361F, 0x2CFB, 0x2457, 0x361F, 0x363F, 0x0A4D, 0x0000, 0x0020, 0x0000, 0x0000, 0x0000, 0x00A3, 0x35BF, 0x35FF, 0x361F, 0x0946, 0x0000, 0x0000, 0x0041, 0x2D5D, 0x361F, 0x35DF, 0x0106, 0x0021, 0x2C56, 0x363F, 0x35DF, 0x35FF, 0x35FF, 0x35DF, 0x363F, 0x23D4},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x334F, 0x669E, 0x667D, 0x4495, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3350, 0x669E, 0x667D, 0x4495, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3A8A, 0x663D, 0x66BE, 0x222A, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x334F, 0x669E, 0x665D, 0x557A, 0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x332F, 0x669E, 0x665D, 0x5538, 0x4495, 0x667E, 0x5E3D, 0x663C, 0x19A7, 0x0000, 0x0000, 0x0000, 0x0000, 0x08A3, 0x5E1C, 0x663D, 0x665D, 0x1146, 0x0000, 0x0000, 0x0041, 0x5DBB, 0x665D, 0x665D, 0x5D99, 0x0882, 0x00A3, 0x5DDB, 0x665D, 0x5E1D, 0x663D, 0x5E3D, 0x669E, 0x3BD2},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BCA, 0xFFB5, 0xFF95, 0xBD2E, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BEB, 0xFFB5, 0xFF95, 0xBD2E, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7B8A, 0xFF75, 0xFF34, 0xFFB5, 0x62A7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BCA, 0xFFB5, 0xFF54, 0xE672, 0x0820, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x83AA, 0xFFB5, 0xFF75, 0xDE11, 0xBD4F, 0xFF95, 0xFF14, 0xFF75, 0xDE72, 0x2103, 0x0000, 0x0020, 0x0000, 0x18C2, 0xFF13, 0xFF54, 0xFF74, 0x3184, 0x0000, 0x0000, 0x0861, 0xEE92, 0xFF75, 0xFF14, 0xFFB6, 0xACCE, 0x0000, 0x41C4, 0xFF34, 0xFF54, 0xFF34, 0xFF34, 0xFF95, 0xA48D},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BCB, 0xFFB5, 0xFF95, 0xBD2F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BEB, 0xFFB5, 0xFF95, 0xBD2F, 0x0000, 0x0020, 0x0000, 0x0840, 0x9C8C, 0xFFD6, 0xFF34, 0xFF14, 0xFFB5, 0x5AA7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BCB, 0xFFB6, 0xFF54, 0xE672, 0x0820, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x83AA, 0xFFB6, 0xFF75, 0xD611, 0xBD4F, 0xFF95, 0xFF34, 0xFF14, 0xFFB5, 0xC590, 0x0000, 0x0000, 0x0000, 0x18C2, 0xF714, 0xFF54, 0xFF75, 0x3184, 0x0000, 0x0000, 0x0861, 0xEE92, 0xFF75, 0xFF34, 0xFF34, 0xFF95, 0x83AA, 0x0000, 0x6B08, 0xFF95, 0xFF34, 0xFF34, 0xFF95, 0x5A87},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BCB, 0xFFB5, 0xFF95, 0xBD2F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BEB, 0xFFB5, 0xFF95, 0xBD2F, 0x0000, 0x0000, 0x18C2, 0xBD8F, 0xFFD6, 0xFF14, 0xFF34, 0xFF34, 0xFFB5, 0x5AA7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BCB, 0xFFB6, 0xFF54, 0xE672, 0x0820, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x83AA, 0xFFB6, 0xFF75, 0xD611, 0xBD4F, 0xFF95, 0xFF34, 0xFF34, 0xFF14, 0xFFB5, 0x8BCA, 0x0000, 0x0000, 0x18C2, 0xF714, 0xFF54, 0xFF75, 0x3184, 0x0000, 0x0000, 0x0861, 0xEE92, 0xFF75, 0xFF34, 0xFF34, 0xFF34, 0xFF55, 0x5266, 0x0000, 0x9C4C, 0xFFD6, 0xFF34, 0xEE92, 0x2102},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BEB, 0xFFB6, 0xFF95, 0xBD4F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BEB, 0xFFB6, 0xFF95, 0xBD4F, 0x0000, 0x2923, 0xDE72, 0xFFB5, 0xFF14, 0xFF54, 0xFF54, 0xFF34, 0xFFB6, 0x5AA7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BEB, 0xFFB6, 0xFF55, 0xE672, 0x0820, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x83CA, 0xFFB6, 0xFF75, 0xD611, 0xBD4F, 0xFF95, 0xFF34, 0xFF54, 0xFF54, 0xFF34, 0xFFB6, 0x5A87, 0x0000, 0x20E2, 0xFF14, 0xFF54, 0xFF75, 0x3184, 0x0000, 0x0000, 0x0861, 0xEEB3, 0xFF75, 0xFF34, 0xFF54, 0xFF34, 0xFF75, 0xF714, 0x2923, 0x0000, 0xBD6F, 0xFFF7, 0x6B08, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x83CA, 0xFF95, 0xFF75, 0xB52E, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x8BEB, 0xFF95, 0xFF75, 0xACEE, 0x41C5, 0xEEF3, 0xFF75, 0xFF14, 0xFF34, 0xFF34, 0xFF34, 0xFF14, 0xFF95, 0x5AA7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x83CA, 0xFF95, 0xFF34, 0xDE52, 0x0820, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x83AA, 0xFF95, 0xFF54, 0xD611, 0xBD4F, 0xFF75, 0xFF14, 0xFF34, 0xFF34, 0xFF14, 0xFF34, 0xEED3, 0x39A4, 0x10A1, 0xF6F3, 0xFF34, 0xFF54, 0x3184, 0x0000, 0x0000, 0x0861, 0xE692, 0xFF54, 0xFF14, 0xFF34, 0xFF34, 0xFF14, 0xFF75, 0xC5B0, 0x39C5, 0x942B, 0x942B, 0x0020, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x18A2, 0x3164, 0x3164, 0x20E2, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x18C2, 0x3164, 0x3164, 0x18E2, 0x2923, 0x3184, 0x2943, 0x3163, 0x3143, 0x3143, 0x3143, 0x2943, 0x3164, 0x1081, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x18A2, 0x3164, 0x3163, 0x2923, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x18A2, 0x3164, 0x3164, 0x2923, 0x2102, 0x3164, 0x2943, 0x3143, 0x3143, 0x3163, 0x2943, 0x3184, 0x2103, 0x0000, 0x2943, 0x3163, 0x3164, 0x0840, 0x0000, 0x0000, 0x0000, 0x2943, 0x3164, 0x3143, 0x3143, 0x3143, 0x3163, 0x2943, 0x3184, 0x39A4, 0x2923, 0x0000, 0x0000, 0x0000},
};

/*
const uint16_t title_block_data[] = {
    (0 << 8 | 0),
    (1 << 8 | 1),
    (2 << 8 | 2),
    (3 << 8 | 1),
    (4 << 8 | 0),
};
const uint8_t title_block_data[][2] = {
    {0, 0},{0, 1},{0, 2},{0, 3},{0, 4},{0, 5},{0, 6},{0, 7},{0, 8},{0, 9},{0, 10},{0, 11},{0, 12},{0, 13},{0, 14},{0, 15},{0, 16},{0, 17},{0, 18},{0, 19},
    {1, 0},{1, 1},{1, 2},{1, 3},{1, 4},{1, 5},{1, 6},{1, 7},{1, 9},{1, 10},{1, 11},{1, 12},{1, 13},{1, 15},{1, 16},{1, 17},{1, 18},{1, 19},
    {2, 0},{2, 1},{2, 2},{2, 5},{2, 6},{2, 7},{2, 12},{2, 16},{2, 17},{2, 18},{2, 19},
    {3, 0},{3, 1},{3, 2},{3, 19},
    {4, 0},{4, 1},{4, 18},{4, 19},
    {5, 0},{5, 1},{5, 2},{5, 19},
    {6, 0},{6, 1},
    {7, 0},{7, 1},
    {8, 0},{8, 1},{8, 19},
    {9, 0},{9, 18},{9, 19},
    {10, 0},{10, 19},
    {11, 0},{11, 1},{11, 2},{11, 18},{11, 19},
    {12, 0},{12, 1},{12, 2},{12, 3},{12, 18},{12, 19},
    {13, 0},{13, 1},{13, 2},{13, 3},{13, 4},{13, 17},{13, 18},{13, 19},
    {14, 0},{14, 1},{14, 2},{14, 3},{14, 18},{14, 19},
    {15, 0},{15, 1},{15, 17},{15, 18},{15, 19},
    {16, 0},{16, 1},{16, 19},
    {17, 0},{17, 3},{17, 4},{17, 5},{17, 18},{17, 19},
    {18, 0},{18, 1},{18, 4},{18, 7},{18, 8},{18, 9},{18, 12},{18, 13},{18, 17},{18, 18},{18, 19},
    {19, 0},{19, 1},{19, 2},{19, 7},{19, 8},{19, 9},{19, 10},{19, 11},{19, 12},{19, 13},{19, 14},{19, 16},{19, 17},{19, 18},{19, 19},
    {20, 0},{20, 1},{20, 2},{20, 3},{20, 5},{20, 6},{20, 7},{20, 8},{20, 9},{20, 10},{20, 11},{20, 12},{20, 13},{20, 14},{20, 15},{20, 16},{20, 17},{20, 18},{20, 19},
    {21, 0},{21, 1},{21, 2},{21, 3},{21, 4},{21, 5},{21, 6},{21, 7},{21, 8},{21, 9},{21, 10},{21, 11},{21, 12},{21, 13},{21, 14},{21, 15},{21, 16},{21, 17},{21, 18},{21, 19},
    {22, 0},{22, 1},{22, 2},{22, 3},{22, 4},{22, 5},{22, 6},{22, 7},{22, 8},{22, 9},{22, 10},{22, 11},{22, 12},{22, 13},{22, 14},{22, 15},{22, 16},{22, 17},{22, 18},{22, 19},
};
*/

static const uint8_t title_block_data[][2] = {{17, 5}, {14, 18}, {17, 19}, {13, 19}, {13, 17}, {22, 1}, {5, 1}, {0, 4}, {0, 8}, {22, 9}, {22, 14}, {1, 3}, {19, 8}, {22, 15}, {19, 7}, {19, 9}, {21, 9}, {1, 19}, {3, 1}, {4, 1}, {13, 0}, {2, 17}, {7, 0}, {22, 8}, {21, 1}, {12, 3}, {18, 18}, {12, 1}, {1, 15}, {20, 15}, {0, 5}, {0, 1}, {21, 8}, {13, 2}, {20, 2}, {1, 1}, {20, 1}, {9, 18}, {2, 2}, {18, 4}, {22, 7}, {19, 0}, {20, 5}, {2, 1}, {0, 7}, {11, 0}, {1, 7}, {15, 18}, {11, 18}, {0, 19}, {7, 1}, {1, 12}, {15, 17}, {0, 12}, {0, 11}, {11, 1}, {21, 14}, {19, 16}, {21, 3}, {13, 18}, {0, 2}, {22, 6}, {20, 3}, {12, 18}, {1, 18}, {18, 19}, {20, 14}, {6, 1}, {4, 0}, {21, 16}, {18, 8}, {18, 9}, {20, 19}, {5, 0}, {18, 13}, {18, 0}, {19, 1}, {0, 3}, {20, 12}, {3, 2}, {19, 17}, {14, 0}, {19, 10}, {22, 17}, {19, 2}, {2, 0}, {22, 0}, {9, 0}, {2, 19}, {16, 1}, {1, 9}, {20, 9}, {0, 14}, {22, 12}, {0, 16}, {5, 19}, {9, 19}, {20, 10}, {19, 13}, {12, 0}, {18, 12}, {20, 8}, {2, 6}, {20, 16}, {11, 2}, {11, 19}, {22, 18}, {2, 18}, {1, 13}, {19, 11}, {22, 19}, {22, 16}, {1, 4}, {13, 1}, {21, 19}, {0, 13}, {21, 12}, {15, 1}, {0, 18}, {21, 15}, {19, 19}, {19, 18}, {0, 6}, {20, 11}, {17, 0}, {1, 6}, {16, 19}, {8, 1}, {0, 0}, {21, 7}, {1, 17}, {18, 7}, {3, 19}, {1, 2}, {21, 10}, {21, 13}, {20, 17}, {22, 4}, {2, 7}, {20, 6}, {15, 19}, {20, 13}, {8, 0}, {13, 3}, {21, 17}, {2, 16}, {1, 10}, {2, 5}, {22, 10}, {20, 0}, {14, 19}, {5, 2}, {22, 3}, {13, 4}, {21, 11}, {8, 19}, {21, 5}, {22, 2}, {12, 19}, {10, 0}, {12, 2}, {0, 17}, {3, 0}, {0, 9}, {21, 6}, {17, 4}, {10, 19}, {4, 18}, {17, 3}, {4, 19}, {17, 18}, {16, 0}, {14, 1}, {6, 0}, {22, 11}, {21, 4}, {1, 11}, {14, 3}, {22, 13}, {22, 5}, {19, 14}, {15, 0}, {1, 0}, {18, 17}, {1, 16}, {14, 2}, {20, 18}, {21, 18}, {21, 0}, {19, 12}, {0, 15}, {0, 10}, {18, 1}, {1, 5}, {21, 2}, {20, 7}, {2, 12}};
static const uint8_t game_block_data[][2] = {{0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {0, 8}, {0, 9}, {0, 10}, {0, 11}, {0, 12}, {0, 13}, {0, 14}, {1, 14}, {2, 14}, {3, 14}, {4, 14}, {5, 14}, {6, 14}, {7, 14}, {8, 14}, {9, 14}, {10, 14}, {11, 14}, {12, 14}, {13, 14}, {14, 14}, {15, 14}, {16, 14}, {17, 14}, {18, 14}, {19, 14}, {20, 14}, {21, 14}, {21, 13}, {21, 12}, {21, 11}, {21, 10}, {21, 9}, {21, 8}, {21, 7}, {21, 6}, {21, 5}, {21, 4}, {21, 3}, {20, 3}, {19, 3}, {18, 3}, {17, 3}, {16, 3}, {15, 3}, {14, 3}, {13, 3}, {12, 3}, {11, 3}, {10, 3}, {9, 3}, {8, 3}, {7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}, {2, 3}, {1, 3}};
static const uint16_t game_block_color_map[][2] = {{BLUE, LIGHTBLUE}, {DARKGREEN, LIGHTGREEN}, {ORANGE, LIGHTORANGE}, {PURPLE, LIGHTPURPLE}, {BROWN, LIGHTBROWN}};

typedef enum
{
    BLOCK_E_I = 0,
    BLOCK_E_LL,
    BLOCK_E_LR,
    BLOCK_E_O,
    BLOCK_E_ZL,
    BLOCK_E_T,
    BLOCK_E_ZR,
    BLOCK_E_NUM
} block_e;

typedef struct
{
    block_e type;

    uint8_t rotation_this;
    uint8_t rotation_last;

    struct
    {
        int16_t x;
        int16_t y;
    } this, last;

    volatile uint16_t *color; // outer, inner
} block_t;

typedef struct
{
    bool exist;
    volatile uint16_t *color; // outer, inner
} game_board_t;

static game_board_t game_board[20][10] = {0};

static const bool block_I[4][4] = {
    {0, 0, 0, 0},
    {1, 1, 1, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
};

static const bool block_LL[3][3] = {
    {1, 0, 0},
    {1, 1, 1},
    {0, 0, 0},
};

static const bool block_LR[3][3] = {
    {0, 0, 1},
    {1, 1, 1},
    {0, 0, 0},
};

static const bool block_O[2][2] = {
    {1, 1},
    {1, 1},
};

static const bool block_ZL[3][3] = {
    {0, 1, 1},
    {1, 1, 0},
    {0, 0, 0},
};

static const bool block_T[3][3] = {
    {0, 1, 0},
    {1, 1, 1},
    {0, 0, 0},
};

static const bool block_ZR[3][3] = {
    {1, 1, 0},
    {0, 1, 1},
    {0, 0, 0},
};

// J、L、S、T、Z 方块踢墙表
static const int8_t JLSTZ_wall_kick_table[4][5][2] = {
    // 0 -> R
    {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
    // R -> 2
    {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
    // 2 -> L
    {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
    // L -> 0
    {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
};

// I 方块踢墙表
static const int8_t I_wall_kick_table[4][5][2] = {
    // 0 -> R
    {{0, 0}, {-2, 0}, {1, 0}, {-2, -1}, {1, 2}},
    // R -> 2
    {{0, 0}, {-1, 0}, {2, 0}, {-1, 2}, {2, -1}},
    // 2 -> L
    {{0, 0}, {2, 0}, {-1, 0}, {2, 1}, {-1, -2}},
    // L -> 0
    {{0, 0}, {1, 0}, {-2, 0}, {1, -2}, {-2, 1}},
};

static const bool *block_type_map[7] = {
    (const bool *)block_I,
    (const bool *)block_LL,
    (const bool *)block_LR,
    (const bool *)block_O,
    (const bool *)block_ZL,
    (const bool *)block_T,
    (const bool *)block_ZR,
};

static const uint8_t block_type_size[7] = {4, 3, 3, 2, 3, 3, 3};

/**
 * @description: 内部调用就行，直接读取方块在相对xy坐标下某一位置是否有方块应该存在
 * @param {bool} *block
 * @param {uint8_t} size
 * @param {uint8_t} rot
 * @param {uint8_t} x
 * @param {uint8_t} y
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-22 06:55:11
 */
static bool block_read_by_rotation(const bool *block, uint8_t size, uint8_t rot, uint8_t x, uint8_t y)
{
    uint8_t newX, newY; // 计算旋转后的坐标
    switch (rot % 4)
    {
    case 0: // 0度旋转
        newX = x;
        newY = y;
        break;
    case 1: // 90度旋转
        newX = y;
        newY = size - 1 - x;
        break;
    case 2: // 180度旋转
        newX = size - 1 - x;
        newY = size - 1 - y;
        break;
    case 3: // 270度旋转
        newX = size - 1 - y;
        newY = x;
        break;
    } // 返回旋转后的位置的值
    return *(block + newY * size + newX);
}

static void block_clear(block_t *_block)
{
    memset(_block, 0, sizeof(block_t));
}

/**
 * @description: 读取block当前的状态
 * @param {block_t} *_block
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-22 06:53:19
 */
static bool block_read_this_base(block_t *_block, uint16_t x, uint16_t y)
{
    return block_read_by_rotation(block_type_map[_block->type], block_type_size[_block->type], _block->rotation_this, x, y);
}

static bool block_read_this_general(block_t *_block, uint16_t x, uint16_t y)
{
    int16_t board_x = x - _block->this.x;
    int16_t board_y = y - _block->this.y;

    if (board_x < 0 || board_x >= block_type_size[_block->type] || board_y < 0 || board_y >= block_type_size[_block->type])
        return false;

    return block_read_this_base(_block, board_x, board_y);
}

/**
 * @description: 读取block上次的状态
 * @param {block_t} *_block
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-22 06:53:10
 */
static bool block_read_last_base(block_t *_block, uint16_t x, uint16_t y)
{
    return block_read_by_rotation(block_type_map[_block->type], block_type_size[_block->type], _block->rotation_last, x, y);
}

static bool block_read_last_general(block_t *_block, uint16_t x, uint16_t y)
{
    int16_t board_x = x - _block->last.x;
    int16_t board_y = y - _block->last.y;

    if (board_x < 0 || board_x >= block_type_size[_block->type] || board_y < 0 || board_y >= block_type_size[_block->type])
        return false;

    return block_read_last_base(_block, board_x, board_y);
}

static void block_put(block_t *_block, uint16_t color_outer, uint16_t color_inner)
{
    for (uint8_t j = 0; j < block_type_size[_block->type]; j++)
        for (uint8_t i = 0; i < block_type_size[_block->type]; i++)
        {
            if (block_read_this_base(_block, i, j))
                draw_colorblock((_block->this.x + 4 + i) * 12, (_block->this.y + 1 + j) * 12, color_outer, color_inner);
        }
}

static void block_erase(block_t *_block)
{
    for (uint8_t j = 0; j < block_type_size[_block->type]; j++)
        for (uint8_t i = 0; i < block_type_size[_block->type]; i++)
        {
            if (block_read_last_base(_block, i, j))
                erase_colorblock((_block->this.x + 4 + i) * 12, (_block->this.y + 1 + j) * 12);
        }
}

static uint8_t block_generate_up_num(block_t *_block)
{
    for (uint8_t j = 0; j < block_type_size[_block->type]; j++)
        for (uint8_t i = 0; i < block_type_size[_block->type]; i++)
        {
            if (block_read_this_base(_block, i, j))
                return j;
        }
    return 3;
}

/**
 * @description: 检测block是否撞到墙，返回true就能放置，false就不能放置
 * @param {block_t} *_block
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-23 01:27:00
 */
static bool block_generate_check(block_t *_block)
{
    for (uint8_t j = 0; j < block_type_size[_block->type]; j++)
        for (uint8_t i = 0; i < block_type_size[_block->type]; i++)
        {
            if (block_read_this_base(_block, i, j) && (game_board[_block->this.y + j][_block->this.x + i].exist || (_block->this.x + i) < 0 || (_block->this.x + i) > 9 || (_block->this.y + j) < 0 || (_block->this.y + j) > 19))
                return false;
        }
    return true;
}

/**
 * @description: block是否已经走不动
 * @param {block_t} *_block
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-23 01:26:46
 */
static bool block_update_check(block_t *_block)
{
    for (uint8_t j = 0; j < block_type_size[_block->type]; j++)
        for (uint8_t i = 0; i < block_type_size[_block->type]; i++)
        {
            if (block_read_this_base(_block, i, j) && (game_board[_block->this.y + j + 1][_block->this.x + i].exist || (_block->this.y + j) > 18))
                return false;
        }
    return true;
}

/**
 * @description: block更新，传入更新方块的内外颜色
 * @param {block_t} *_block
 * @param {uint16_t} color_outer
 * @param {uint16_t} color_inner
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-24 04:40:22
 */
static void block_update(block_t *_block, uint16_t color_outer, uint16_t color_inner)
{
    for (uint8_t j = 0; j < block_type_size[_block->type]; j++)
        for (uint8_t i = 0; i < block_type_size[_block->type]; i++)
        {
            if (block_read_last_base(_block, i, j) && (_block->last.x + i) < 10 && (_block->last.x + i) >= 0 && (_block->last.y + j) < 20)
                if (!(block_read_this_general(_block, _block->last.x + i, _block->last.y + j)))
                    erase_colorblock((_block->last.x + 4 + i) * 12, (_block->last.y + 1 + j) * 12);
            if (block_read_this_base(_block, i, j))
                if (!(block_read_last_general(_block, _block->this.x + i, _block->this.y + j)))
                    draw_colorblock((_block->this.x + 4 + i) * 12, (_block->this.y + 1 + j) * 12, color_outer, color_inner);
        }
}

// game_board_t game_board【20】【10】

static void board_clear()
{
    memset(game_board, 0, sizeof(game_board));
}
/**
 * @description: 将block方块添加到board棋盘中
 * @param {block_t} *_block
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-22 21:57:01
 */
static void board_add_block(block_t *_block)
{
    for (uint8_t j = 0; j < block_type_size[_block->type]; j++)
        for (uint8_t i = 0; i < block_type_size[_block->type]; i++)
            if (block_read_this_base(_block, i, j))
            {
                game_board[_block->this.y + j][_block->this.x + i].exist = true;
                game_board[_block->this.y + j][_block->this.x + i].color = _block->color;
            }
}

/**
 * @description: 重新摆放棋盘内所有方块，不建议在游戏中使用，正确用法比如load game或者pause后的恢复刷屏
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-24 04:41:11
 */
static void board_put_all()
{
    for (uint8_t j = 0; j < 20; j++)
        for (uint8_t i = 0; i < 10; i++)
            if (game_board[j][i].exist)
                draw_colorblock((4 + i) * 12, (1 + j) * 12, game_board[j][i].color[0], game_board[j][i].color[1]);
            else
                erase_colorblock((4 + i) * 12, (1 + j) * 12);
}

/**
 * @description: 将position y处的棋盘整体下移
 * @param {uint8_t} down_pos_y
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-24 04:42:53
 */
static void board_move_down(uint8_t down_pos_y)
{
    for (int8_t j = down_pos_y; j > 1; j--)
    {
        for (uint8_t i = 0; i < 10; i++)
        {
            if (game_board[j - 1][i].exist)
            {
                if (j - 1 - 1 < 0 || !game_board[j - 1 - 1][i].exist)
                    erase_colorblock((4 + i) * 12, (1 + j - 1) * 12);

                if (game_board[j][i].color != game_board[j - 1][i].color)
                    draw_colorblock((4 + i) * 12, (1 + j) * 12, game_board[j - 1][i].color[0], game_board[j - 1][i].color[1]);
            }
            else if (game_board[j][i].exist)
                erase_colorblock((4 + i) * 12, (1 + j) * 12);

            // 消除行逻辑写好之后要剪这里
            game_board[j][i].exist = game_board[j - 1][i].exist;
            game_board[j][i].color = game_board[j - 1][i].color;
        }
    }

    // board_put_all();
}

/**
 * @description: 结算部分，消除被填满的行并返回本次消除的行数
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-23 02:47:51
 */
static uint8_t board_calculate()
{
    uint8_t num = 0;
    for (int8_t j = 19; j >= 0; j--)
    {
        bool board_rol_exist = true;
        for (uint8_t i = 0; i < 10; i++)
        {
            if (!game_board[j][i].exist)
            {
                board_rol_exist = false;
                break;
            }
        }

        if (board_rol_exist)
        {
            num++;
            // 此处j++是因为当前行被消灭了，下一行已经来到了当前行
            board_move_down(j++);
        }
    }
    return num;
}

static bool add_new_block_flag = false;
static void game_title_background();
static void game_title();
static void game_over();
static block_t block_pre_add, block;

static uint16_t game_score = 0;
static uint8_t game_level = 1;
/**
 * @description: 这个是游戏的主逻辑函数
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-22 07:40:47
 */
static void game_main()
{
    static uint32_t title_time = 0;
    static uint32_t lastTime = 0;
    uint32_t currentTime;

    currentTime = UTIL_GetTick(); // 获取当前时间（毫秒）

    if (currentTime - lastTime >= 1000 / game_level)
    {
        if (!block_update_check(&block) && add_new_block_flag == false)
        {
            add_new_block_flag = true;

            board_add_block(&block);
        }

        if (add_new_block_flag)
        {
            add_new_block_flag = false;

            block.type = block_pre_add.type;
            block.rotation_last = block_pre_add.rotation_last;
            block.rotation_this = block_pre_add.rotation_this;
            block.last.x = 5 - block_type_size[block.type] / 2;
            block.this.x = 5 - block_type_size[block.type] / 2;

            block.last.y = -block_generate_up_num(&block);
            block.this.y = block.last.y;

            block.color = block_pre_add.color;

            // 游戏结束
            if (!block_generate_check(&block))
            {
                ST7789_WriteString(40, 180, "GAME OVER!", Font_16x26, RED, BLACK);
                ST7789_WriteString(28, 210, "LONG PRESS ANY KEY TO BACK", Font_7x10, GREEN, BLACK);

                // 暂时先用着
                ST7789_WriteString(50, 20, "SCORE", Font_16x26, WHITE, BLACK);
                ST7789_WriteNum_Int(50, 50, game_score, Font_16x26, WHITE, BLACK);

                game_score = 0;
                game_level = 1;

                current_func = game_over;
                return;
            }

            block_put(&block, block.color[0], block.color[1]);

            block_erase(&block_pre_add);
            srand(UTIL_GetTick();

            uint8_t block_type_rand_protector = rand() % BLOCK_E_NUM;
            while (block_type_rand_protector == block.type)
            {
                srand(UTIL_GetTick());
                block_type_rand_protector = rand() % BLOCK_E_NUM;
            }

            volatile uint16_t *block_color_rand_protector = (volatile uint16_t *)game_block_color_map[rand() % (sizeof(game_block_color_map) / sizeof(uint16_t[2]))];
            while (block_color_rand_protector == block.color)
            {
                srand(UTIL_GetTick());
                block_color_rand_protector = (volatile uint16_t *)game_block_color_map[rand() % (sizeof(game_block_color_map) / sizeof(uint16_t[2]))];
            }
            block_pre_add.type = block_type_rand_protector;
            block_pre_add.rotation_last = 0;
            block_pre_add.rotation_this = block_pre_add.rotation_last;
            block_pre_add.last.x = 12;
            block_pre_add.last.y = 3;
            block_pre_add.this.x = block_pre_add.last.x;
            block_pre_add.this.y = block_pre_add.last.y;
            block_pre_add.color = (volatile uint16_t *)block_color_rand_protector;

            block_put(&block_pre_add, block_pre_add.color[0], block_pre_add.color[1]);
        }
        else
        {
            block.rotation_last = block.rotation_this;
            block.last.x = block.this.x;
            block.last.y = block.this.y;

            block.this.y++;

            if (!block_generate_check(&block))
                block.this.y = block.last.y;
            else
                block_update(&block, block.color[0], block.color[1]);
        }

        // test
        // board_put_all();

        uint16_t score_num = board_calculate();
        game_score += score_num * score_num;
        if (game_level < 10)
            game_level = game_score / 10 + 1;

        ST7789_WriteString(180, 140, "SCORE", Font_11x18, BLUE, BLACK);
        ST7789_WriteNum_Int(180, 160, game_score, Font_11x18, WHITE, BLACK);

        ST7789_WriteString(180, 200, "LEVEL", Font_11x18, RED, BLACK);
        ST7789_WriteNum_Int(180, 220, game_level, Font_11x18, WHITE, BLACK);

        lastTime = currentTime;
    }

    if (key_get_state(KEY_A) == KEY_SHORT_PRESS || key_get_state(KEY_A) == KEY_LONG_PRESS)
    {
        key_clear_state(KEY_A);
        block.rotation_last = block.rotation_this;
        block.last.x = block.this.x;
        block.last.y = block.this.y;

        if (block.rotation_this > 0)
            block.rotation_this = block.rotation_this - 1;
        else
            block.rotation_this = 3;

        // SRS踢墙规则
        bool can_block_generate = false;
        if (block.type == BLOCK_E_I)
            for (uint8_t i = 0; i < 5; i++)
            {
                if (!block_generate_check(&block))
                {
                    block.this.x = block.last.x - I_wall_kick_table[block.rotation_this][i][0];
                    block.this.y = block.last.y + I_wall_kick_table[block.rotation_this][i][1];
                }
                else
                {
                    can_block_generate = true;
                    break;
                }
            }
        else if (block.type != BLOCK_E_O)
            for (uint8_t i = 0; i < 5; i++)
            {
                if (!block_generate_check(&block))
                {
                    block.this.x = block.last.x - JLSTZ_wall_kick_table[block.rotation_this][i][0];
                    block.this.y = block.last.y + JLSTZ_wall_kick_table[block.rotation_this][i][1];
                }
                else
                {
                    can_block_generate = true;
                    break;
                }
            }
        else if (block_generate_check(&block))
            can_block_generate = true;
        if (!can_block_generate)
        {
            block.rotation_this = block.rotation_last;
            block.this.x = block.last.x;
            block.this.y = block.last.y;
        }
        else
            block_update(&block, block.color[0], block.color[1]);
    }

    if (key_get_state(KEY_B) == KEY_SHORT_PRESS || key_get_state(KEY_B) == KEY_LONG_PRESS)
    {
        key_clear_state(KEY_B);
        block.rotation_last = block.rotation_this;
        block.last.x = block.this.x;
        block.last.y = block.this.y;

        if (block.rotation_this < 3)
            block.rotation_this = block.rotation_this + 1;
        else
            block.rotation_this = 0;

        // SRS踢墙规则
        bool can_block_generate = false;
        if (block.type == BLOCK_E_I)
            for (uint8_t i = 0; i < 5; i++)
            {
                if (!block_generate_check(&block))
                {
                    block.this.x = block.last.x + I_wall_kick_table[block.rotation_last][i][0];
                    block.this.y = block.last.y - I_wall_kick_table[block.rotation_last][i][1];
                }
                else
                {
                    can_block_generate = true;
                    break;
                }
            }
        else if (block.type != BLOCK_E_O)
            for (uint8_t i = 0; i < 5; i++)
            {
                if (!block_generate_check(&block))
                {
                    block.this.x = block.last.x + JLSTZ_wall_kick_table[block.rotation_last][i][0];
                    block.this.y = block.last.y - JLSTZ_wall_kick_table[block.rotation_last][i][1];
                }
                else
                {
                    can_block_generate = true;
                    break;
                }
            }
        else if (block_generate_check(&block))
            can_block_generate = true;
        if (!can_block_generate)
        {
            block.rotation_this = block.rotation_last;
            block.this.x = block.last.x;
            block.this.y = block.last.y;
        }
        else
            block_update(&block, block.color[0], block.color[1]);
    }

    if (key_get_state(KEY_LEFT) == KEY_SHORT_PRESS || key_get_state(KEY_LEFT) == KEY_LONG_PRESS)
    {
        key_clear_state(KEY_LEFT);
        block.rotation_last = block.rotation_this;
        block.last.x = block.this.x;
        block.last.y = block.this.y;

        block.this.x--;

        if (!block_generate_check(&block))
            block.this.x = block.last.x;
        else
            block_update(&block, block.color[0], block.color[1]);
    }
    if (key_get_state(KEY_RIGHT) == KEY_SHORT_PRESS || key_get_state(KEY_RIGHT) == KEY_LONG_PRESS)
    {
        key_clear_state(KEY_RIGHT);
        block.rotation_last = block.rotation_this;
        block.last.x = block.this.x;
        block.last.y = block.this.y;

        block.this.x++;

        if (!block_generate_check(&block))
            block.this.x = block.last.x;
        else
            block_update(&block, block.color[0], block.color[1]);
    }

    // if (key_get_state(KEY_UP) == KEY_SHORT_PRESS || key_get_state(KEY_UP) == KEY_LONG_PRESS)
    // {
    //     key_clear_state(KEY_UP);
    //     block.rotation_last = block.rotation_this;
    //     block.last.x = block.this.x;
    //     block.last.y = block.this.y;

    //     block.this.y--;

    //     if (!block_generate_check(&block))
    //         block.this.y = block.last.y;
    //     else
    //     {
    //         block_update(&block, block.color[0], block.color[1]);

    //         lastTime = currentTime;
    //     }
    // }
    if (key_get_state(KEY_DOWN) == KEY_SHORT_PRESS || key_get_state(KEY_DOWN) == KEY_LONG_PRESS)
    {
        key_clear_state(KEY_DOWN);
        block.rotation_last = block.rotation_this;
        block.last.x = block.this.x;
        block.last.y = block.this.y;

        block.this.y++;

        if (!block_generate_check(&block))
            block.this.y = block.last.y;
        else
        {
            block_update(&block, block.color[0], block.color[1]);

            lastTime = currentTime;
        }
    }

    if (key_get_state(KEY_X) == KEY_LONG_PRESS && key_get_state(KEY_Y) == KEY_LONG_PRESS)
    {
        key_clear_all_state();
        add_new_block_flag = false;

        game_score = 0;
        game_level = 1;

        ST7789_Fill_Color(BLACK);
        game_title_background();
        current_func = game_title;
    }
}

static void game_start()
{
    srand(UTIL_GetTick());
    ST7789_Fill_Color(BLACK);

    board_clear();
    block_clear(&block);
    block_clear(&block_pre_add);
    for (uint8_t i = 0; i < sizeof(game_block_data) / sizeof(uint8_t[2]); i++)
        draw_colorblock(game_block_data[i][1] * 12, game_block_data[i][0] * 12, GRAY, LGRAY);

    add_new_block_flag = true;

    block_pre_add.type = rand() % BLOCK_E_NUM;
    block_pre_add.rotation_last = 0;
    block_pre_add.rotation_this = block_pre_add.rotation_last;
    block_pre_add.last.x = 12;
    block_pre_add.last.y = 3;
    block_pre_add.this.x = block_pre_add.last.x;
    block_pre_add.this.y = block_pre_add.last.y;

    // block_pre_add.color_outer = BLUE;
    // block_pre_add.color_inner = LIGHTBLUE;
    block_pre_add.color = (volatile uint16_t *)game_block_color_map[rand() % (sizeof(game_block_color_map) / sizeof(uint16_t[2]))];
    current_func = game_main;
}

static void game_over()
{

    if (key_long_press(KEY_UP) || key_long_press(KEY_DOWN) || key_long_press(KEY_LEFT) || key_long_press(KEY_RIGHT) || key_long_press(KEY_A) || key_long_press(KEY_B) || key_long_press(KEY_X) || key_long_press(KEY_Y))
    {
        key_clear_all_state();
        add_new_block_flag = false;

        ST7789_Fill_Color(BLACK);
        game_title_background();
        current_func = game_title;
    }
}

static void game_quit()
{
    game_quit_flag = true;
}
static const menu_select_t game_menu_opt[] =
    {
        {"NEW GAME", game_start},
        {"LOAD GAME", NULL},
        {"LEADER BOARD", NULL},
        {"SETTINGS", NULL},
        {"QUIT", game_quit},
};
static const uint8_t game_menu_opt_num = sizeof(game_menu_opt) / sizeof(menu_select_t);

/**
 * @description: 游戏开始页面菜单
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-22 21:52:28
 */
static void game_menu()
{
    static uint8_t main_options = 0;

    // 换页
    if (key_short_press(KEY_UP))
    {
        uint8_t last_option = main_options;
        if (main_options-- == 0)
            main_options = game_menu_opt_num - 1;

        ST7789_WriteString((240 - strlen(game_menu_opt[last_option].name) * 11) / 2, 100 + 18 * last_option, game_menu_opt[last_option].name, Font_11x18, WHITE, BLACK);
        ST7789_WriteString((240 - strlen(game_menu_opt[main_options].name) * 11) / 2, 100 + 18 * main_options, game_menu_opt[main_options].name, Font_11x18, BLACK, RED);
    }
    if (key_short_press(KEY_DOWN))
    {
        uint8_t last_option = main_options;
        if (++main_options >= game_menu_opt_num)
            main_options = 0;

        ST7789_WriteString((240 - strlen(game_menu_opt[last_option].name) * 11) / 2, 100 + 18 * last_option, game_menu_opt[last_option].name, Font_11x18, WHITE, BLACK);
        ST7789_WriteString((240 - strlen(game_menu_opt[main_options].name) * 11) / 2, 100 + 18 * main_options, game_menu_opt[main_options].name, Font_11x18, BLACK, RED);
    }

    if (key_short_press(KEY_A))
    {
        for (uint8_t i = 0; i < game_menu_opt_num; i++)
        {
            ST7789_WriteString((240 - strlen(game_menu_opt[i].name) * 11) / 2, 100 + 18 * i, game_menu_opt[i].name, Font_11x18, BLACK, BLACK);
        }
        current_func = game_menu_opt[main_options].func;
        main_options = 0;
    }
}

/**
 * @description: 游戏标题页面，PUSH START
 * @return {*}
 * @Author: Calvaria
 * @Date: 2025-01-22 21:52:45
 */
static void game_title()
{
    static uint32_t title_time = 0;
    static uint32_t lastTime = 0;
    uint32_t currentTime;

    currentTime = UTIL_GetTick(); // 获取当前时间（毫秒）
    if (currentTime - lastTime >= 500)
    {
        static bool toggle = true;

        if (toggle)
        {
            ST7789_WriteString(40, 180, "PUSH START", Font_16x26, WHITE, BLACK);
            toggle = false;
        }
        else
        {
            ST7789_WriteString(40, 180, "PUSH START", Font_16x26, BLACK, BLACK);
            toggle = true;
        }
        lastTime = currentTime;
    }

    if (key_short_press(KEY_UP) || key_short_press(KEY_DOWN) || key_short_press(KEY_LEFT) || key_short_press(KEY_RIGHT) || key_short_press(KEY_A) || key_short_press(KEY_B) || key_short_press(KEY_X) || key_short_press(KEY_Y))
    {
        key_clear_all_state();
        ST7789_WriteString(40, 180, "PUSH START", Font_16x26, BLACK, BLACK);

        for (uint8_t i = 0; i < game_menu_opt_num; i++)
        {
            ST7789_WriteString((240 - strlen(game_menu_opt[i].name) * 11) / 2, 100 + 18 * i, game_menu_opt[i].name, Font_11x18, WHITE, BLACK);
        }
        ST7789_WriteString((240 - strlen(game_menu_opt[0].name) * 11) / 2, 100 + 18 * 0, game_menu_opt[0].name, Font_11x18, BLACK, RED);

        current_func = game_menu;
    }
}

static void game_title_background()
{
    for (uint8_t i = 0; i < 24; i++)
        for (uint8_t j = 0; j < 80; j++)
            ST7789_Fill(j * 2 + 40, i * 2 + 40, j * 2 + 42, i * 2 + 42, title_image_data[i][j]);

    for (uint8_t i = 0; i < sizeof(title_block_data) / sizeof(uint8_t[2]); i++)
        draw_colorblock(title_block_data[i][1] * 12, title_block_data[i][0] * 12, GRAY, LGRAY);
}

static void (*current_func)() = game_title;
void game_tetris_interface()
{
    game_title_background();

    do
    {
        current_func();

        DisplayFrameRate();
    } while (!game_quit_flag);

    game_quit_flag = false;
    current_func = game_title;
    back_to_parent();
}