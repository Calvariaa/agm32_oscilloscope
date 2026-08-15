#include "main.h"
#include "ips200.h"
#include "util.h"

int main(void)
{
  board_init();

  INT_SetIRQThreshold(MIN_IRQ_PRIORITY);

  ips200_init();

  ips200_clear();

  ips200_full(IPS200_DEFAULT_BGCOLOR);

  uint16_t data[128];
  int16_t data_index = 0;
  for( ; 64 > data_index; data_index ++)
    data[data_index] = data_index;
  for(data_index = 64; 128 > data_index; data_index ++)
    data[data_index] = 128 - data_index;

  while (1) {

    // 显示的 flaot 数据 最多显示 8bit 位整数 最多显示 6bit 位小数
    ips200_show_float(  0 , 16*8,   -13.141592,     1, 6);                  // 显示 float 数据 1bit 整数 6bit 小数 应当显示 -3.141592 总共会有 9 个字符的显示占位
    ips200_show_float(  80, 16*8,   13.141592,      8, 4);                  // 显示 float 数据 8bit 整数 4bit 小数 应当显示 13.1415 总共会有 14 个字符的显示占位 后面会有 5 个字符的空白占位

    ips200_show_int(    0 , 16*9,   -127,           2);                     // 显示 int8 数据
    ips200_show_uint(   80, 16*9,   255,            4);                     // 显示 uint8 数据

    ips200_show_int(    0 , 16*10,  -32768,         4);                     // 显示 int16 数据
    ips200_show_uint(   80, 16*10,  65535,          6);                     // 显示 uint16 数据

    ips200_show_int(    0 , 16*11,  -2147483648,    8);                     // 显示 int32 数据 8bit 整数 应当显示 -47483648
    ips200_show_uint(   80, 16*11,  4294967295,     8);                     // 显示 uint32 数据 10bit 整数 应当显示 4294967295

    
    UTIL_IdleMs(1000);

    ips200_full(RGB565_GRAY);
    ips200_show_wave(88, 144, data, 128, 64,  64, 32);                      // 显示一个三角波形 波形宽度 128 波形最大值 64 显示宽度 64 显示最大值 32
    UTIL_IdleMs(1000);
    ips200_full(RGB565_GRAY);
    ips200_show_wave(56, 128, data, 128, 64, 128, 64);                      // 显示一个三角波形 波形宽度 128 波形最大值 64 显示宽度 128 显示最大值 64
    UTIL_IdleMs(1000);

    // 使用画线函数 从顶上两个角画射线
    ips200_clear();
    for(data_index = 0; 240 > data_index; data_index += 10)
    {
      ips200_draw_line(0, 0, data_index, 320 - 1, 0x66CCFF);
    UTIL_IdleMs(20);
    }
    ips200_draw_line(0, 0, 240 - 1, 320 - 1, 0x66CCFF);
    for(data_index = 310; 0 <= data_index; data_index -= 10)
    {
      ips200_draw_line(0, 0, 240 - 1, data_index, 0x66CCFF);
    UTIL_IdleMs(20);
    }

    ips200_draw_line(240 - 1, 0, 239, 320 - 1, 0x66CCFF);
    for(data_index = 230; 0 <= data_index; data_index -= 10)
    {
      ips200_draw_line(240 - 1, 0, data_index, 320 - 1, 0x66CCFF);
    UTIL_IdleMs(20);
    }
    ips200_draw_line(240 - 1, 0, 0, 320 - 1, 0x66CCFF);
    for(data_index = 310; 0 <= data_index; data_index -= 10)
    {
      ips200_draw_line(240 - 1, 0, 0, data_index, 0x66CCFF);
    UTIL_IdleMs(20);
    }
    UTIL_IdleMs(1000);
    
    GPIO_Toggle(LED_GPIO, LED_GPIO_BIT);
  }
}
