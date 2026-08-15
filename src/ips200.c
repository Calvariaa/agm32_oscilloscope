#include "ips200.h"
#include "font.h"
#include "spi.h"
#include "gpio.h"
#include "util.h"
#include "dmac.h"
#include <string.h>
#include <stddef.h>
#include <math.h>

uint16_t ips200_width_max  = 320;
uint16_t ips200_height_max = 240;

static uint16_t          ips200_pencolor    = IPS200_DEFAULT_PENCOLOR;
static uint16_t          ips200_bgcolor     = IPS200_DEFAULT_BGCOLOR;
static ips200_dir_enum   ips200_display_dir = IPS200_DEFAULT_DISPLAY_DIR;
static ips200_font_size_enum ips200_display_font = IPS200_DEFAULT_DISPLAY_FONT;

// ------------------------------------------------------------------ low-level
static uint32_t ips_dma_buf[1024];  // 4096 bytes, naturally 4-byte aligned
static volatile uint32_t ips_dma_active = 0;

static void ips_dma_isr(void)
{
    SPI_ClearInt(SPI1);
    DMAC_DisableChannel(DMAC_CHANNEL0);
    ips_dma_active = 0;
}

void SPI1_isr(void) __attribute__((weak, alias("ips_dma_isr")));

static void ips_stream_send(const uint8_t *bytes, uint32_t nbytes)
{
    while (ips_dma_active);

    if (nbytes <= 4) {
        for (uint32_t i = 0; i < nbytes; i++) {
            SPI_SetPhaseCtrl(SPI1, SPI_PHASE_0, SPI_PHASE_ACTION_TX, SPI_PHASE_MODE_SINGLE, 1);
            SPI_SetPhaseData(SPI1, SPI_PHASE_0, bytes[i]);
            SPI_Start(SPI1, SPI_CTRL_PHASE_CNT1, SPI_CTRL_DMA_OFF, SPI_INTERRUPT_OFF);
            SPI_WaitForDone(SPI1);
        }
        return;
    }

    memcpy(ips_dma_buf, bytes, nbytes);

    DMAC_DisableSyncRequest(SPI_TX_DMA_REQ(SPI1));
    DMAC_DisableSyncRequest(SPI_RX_DMA_REQ(SPI1));

    const uint8_t *b = (const uint8_t *)ips_dma_buf;
    uint32_t txData = (uint32_t)b[0] | ((uint32_t)b[1] << 8)
                    | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    SPI_SetPhaseCtrl(SPI1, SPI_PHASE_0, SPI_PHASE_ACTION_TX, SPI_PHASE_MODE_SINGLE, 4);
    SPI_SetPhaseData(SPI1, SPI_PHASE_0, txData);

    SPI_SetPhaseCtrl(SPI1, SPI_PHASE_1, SPI_PHASE_ACTION_TX, SPI_PHASE_MODE_SINGLE, nbytes - 4);
    DMAC_Config(DMAC_CHANNEL0,
                (uint32_t)ips_dma_buf + 4, (uint32_t)&SPI1->PHASE_DATA[SPI_PHASE_1],
                DMAC_ADDR_INCR_ON, DMAC_ADDR_INCR_OFF,
                DMAC_WIDTH_32_BIT, DMAC_WIDTH_32_BIT,
                DMAC_BURST_1, DMAC_BURST_1, 0,
                DMAC_MEM_TO_PERIPHERAL_PERIPHERAL_CTRL,
                0, SPI_TX_DMA_REQ(SPI1));

    ips_dma_active = 1;
    SPI_Start(SPI1, SPI_CTRL_PHASE_CNT2, SPI_CTRL_DMA_ON, SPI_INTERRUPT_ON);
}

static void spi_write_byte(uint8_t b)
{
    SPI_SetPhaseCtrl(SPI1, SPI_PHASE_0, SPI_PHASE_ACTION_TX, SPI_PHASE_MODE_SINGLE, 1);
    SPI_SetPhaseData(SPI1, SPI_PHASE_0, (uint32_t)b);
    SPI_Start(SPI1, SPI_CTRL_PHASE_CNT1, SPI_CTRL_DMA_OFF, SPI_INTERRUPT_OFF);
    SPI_WaitForDone(SPI1);
}

static void spi_write_word(uint16_t w)
{
    uint32_t data = ((uint32_t)(w & 0xFF) << 8) | (w >> 8);
    SPI_SetPhaseCtrl(SPI1, SPI_PHASE_0, SPI_PHASE_ACTION_TX, SPI_PHASE_MODE_SINGLE, 2);
    SPI_SetPhaseData(SPI1, SPI_PHASE_0, data);
    SPI_Start(SPI1, SPI_CTRL_PHASE_CNT1, SPI_CTRL_DMA_OFF, SPI_INTERRUPT_OFF);
    SPI_WaitForDone(SPI1);
}

static void ips200_write_command(uint8_t cmd)
{
    IPS200_DC_LOW();
    spi_write_byte(cmd);
    IPS200_DC_HIGH();
}

static void ips200_write_8bit_data(uint8_t dat)
{
    spi_write_byte(dat);
}

void ips200_write_16bit_data(uint16_t dat)
{
    spi_write_word(dat);
}

static void ips200_write_16bit_data_array(const uint16_t *dat, uint32_t len)
{
    uint32_t buf_size = sizeof(ips_dma_buf);
    uint8_t *dst = (uint8_t *)ips_dma_buf;
    uint32_t src_off = 0;
    uint32_t remaining = len;
    while (remaining > 0) {
        uint32_t nw = remaining > (buf_size / 2) ? (buf_size / 2) : remaining;
        for (uint32_t i = 0; i < nw; i++) {
            uint16_t w = dat[src_off + i];
            dst[i * 2]     = (uint8_t)(w >> 8);
            dst[i * 2 + 1] = (uint8_t)(w & 0xFF);
        }
        // wait, then kick off DMA directly without going through ips_stream_send
        // (buffer is already in ips_dma_buf, no memcpy needed)
        while (ips_dma_active);
        uint32_t nbytes = nw * 2;
        if (nbytes <= 4) {
            for (uint32_t i = 0; i < nbytes; i++) {
                SPI_SetPhaseCtrl(SPI1, SPI_PHASE_0, SPI_PHASE_ACTION_TX, SPI_PHASE_MODE_SINGLE, 1);
                SPI_SetPhaseData(SPI1, SPI_PHASE_0, dst[i]);
                SPI_Start(SPI1, SPI_CTRL_PHASE_CNT1, SPI_CTRL_DMA_OFF, SPI_INTERRUPT_OFF);
                SPI_WaitForDone(SPI1);
            }
        } else {
            DMAC_DisableSyncRequest(SPI_TX_DMA_REQ(SPI1));
            DMAC_DisableSyncRequest(SPI_RX_DMA_REQ(SPI1));
            uint32_t txData = (uint32_t)dst[0] | ((uint32_t)dst[1] << 8)
                            | ((uint32_t)dst[2] << 16) | ((uint32_t)dst[3] << 24);
            SPI_SetPhaseCtrl(SPI1, SPI_PHASE_0, SPI_PHASE_ACTION_TX, SPI_PHASE_MODE_SINGLE, 4);
            SPI_SetPhaseData(SPI1, SPI_PHASE_0, txData);
            SPI_SetPhaseCtrl(SPI1, SPI_PHASE_1, SPI_PHASE_ACTION_TX, SPI_PHASE_MODE_SINGLE, nbytes - 4);
            DMAC_Config(DMAC_CHANNEL0,
                        (uint32_t)ips_dma_buf + 4, (uint32_t)&SPI1->PHASE_DATA[SPI_PHASE_1],
                        DMAC_ADDR_INCR_ON, DMAC_ADDR_INCR_OFF,
                        DMAC_WIDTH_32_BIT, DMAC_WIDTH_32_BIT,
                        DMAC_BURST_1, DMAC_BURST_1, 0,
                        DMAC_MEM_TO_PERIPHERAL_PERIPHERAL_CTRL,
                        0, SPI_TX_DMA_REQ(SPI1));
            ips_dma_active = 1;
            SPI_Start(SPI1, SPI_CTRL_PHASE_CNT2, SPI_CTRL_DMA_ON, SPI_INTERRUPT_ON);
        }
        src_off += nw;
        remaining -= nw;
    }
    while (ips_dma_active);
}

static void ips200_write_8bit_data_array(const uint8_t *dat, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        spi_write_byte(dat[i]);
    }
}

static void ips200_set_region(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    ips200_write_command(0x2A);
    spi_write_word(x1);
    spi_write_word(x2);

    ips200_write_command(0x2B);
    spi_write_word(y1);
    spi_write_word(y2);

    ips200_write_command(0x2C);
}

// ------------------------------------------------------------------ public API
void ips200_set_dir(ips200_dir_enum dir)
{
    ips200_display_dir = dir;
    if (dir == IPS200_PORTAIT || dir == IPS200_PORTAIT_180) {
        ips200_width_max  = 240;
        ips200_height_max = 320;
    } else {
        ips200_width_max  = 320;
        ips200_height_max = 240;
    }
}

void ips200_set_font(ips200_font_size_enum font)
{
    ips200_display_font = font;
}

void ips200_set_color(uint16_t pen, uint16_t bgcolor)
{
    ips200_pencolor = pen;
    ips200_bgcolor  = bgcolor;
}

void ips200_clear(void)
{
    ips200_set_region(0, 0, ips200_width_max - 1, ips200_height_max - 1);
    uint32_t nbytes = (uint32_t)ips200_width_max * ips200_height_max * 2;
    uint32_t buf_size = sizeof(ips_dma_buf);
    uint8_t *dst = (uint8_t *)ips_dma_buf;
    uint8_t hi = (uint8_t)(ips200_bgcolor >> 8), lo = (uint8_t)(ips200_bgcolor & 0xFF);
    uint32_t nw = buf_size / 2;
    for (uint32_t i = 0; i < nw; i++) { dst[i * 2] = hi; dst[i * 2 + 1] = lo; }
    while (nbytes > 0) {
        uint32_t chunk = nbytes > buf_size ? buf_size : nbytes;
        while (ips_dma_active);
        ips_stream_send((const uint8_t *)ips_dma_buf, chunk);
        nbytes -= chunk;
    }
    while (ips_dma_active);
}

void ips200_full(uint16_t color)
{
    ips200_set_region(0, 0, ips200_width_max - 1, ips200_height_max - 1);
    uint32_t nbytes = (uint32_t)ips200_width_max * ips200_height_max * 2;
    uint32_t buf_size = sizeof(ips_dma_buf);
    uint8_t *dst = (uint8_t *)ips_dma_buf;
    uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)(color & 0xFF);
    uint32_t nw = buf_size / 2;
    for (uint32_t i = 0; i < nw; i++) { dst[i * 2] = hi; dst[i * 2 + 1] = lo; }
    while (nbytes > 0) {
        uint32_t chunk = nbytes > buf_size ? buf_size : nbytes;
        while (ips_dma_active);
        ips_stream_send((const uint8_t *)ips_dma_buf, chunk);
        nbytes -= chunk;
    }
    while (ips_dma_active);
}

void ips200_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    ips200_set_region(x, y, x, y);
    ips200_write_16bit_data(color);
}

void ips200_draw_line(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, uint16_t color)
{
    int16_t x_dir = (x_start < x_end) ? 1 : -1;
    int16_t y_dir = (y_start < y_end) ? 1 : -1;
    float temp_rate = 0, temp_b = 0;

    do {
        if (x_start != x_end) {
            temp_rate = (float)(y_start - y_end) / (float)(x_start - x_end);
            temp_b = (float)y_start - (float)x_start * temp_rate;
        } else {
            while (y_start != y_end) {
                ips200_draw_point(x_start, y_start, color);
                y_start += y_dir;
            }
            ips200_draw_point(x_start, y_start, color);
            break;
        }
        int16_t dy = y_start > y_end ? (y_start - y_end) : (y_end - y_start);
        int16_t dx = x_start > x_end ? (x_start - x_end) : (x_end - x_start);
        if (dy > dx) {
            while (y_start != y_end) {
                ips200_draw_point(x_start, y_start, color);
                y_start += y_dir;
                x_start = (int16_t)(((float)y_start - temp_b) / temp_rate);
            }
        } else {
            while (x_start != x_end) {
                ips200_draw_point(x_start, y_start, color);
                x_start += x_dir;
                y_start = (int16_t)((float)x_start * temp_rate + temp_b);
            }
        }
        ips200_draw_point(x_start, y_start, color);
    } while (0);
}

// ------------------------------------------------------------------ text
void ips200_show_char(uint16_t x, uint16_t y, char dat)
{
    uint8_t i, j;
    switch (ips200_display_font) {
        case IPS200_6X8_FONT: {
            uint16_t buf[6 * 8];
            ips200_set_region(x, y, x + 5, y + 7);
            for (i = 0; i < 6; i++) {
                uint8_t tmp = ascii_font_6x8[(uint8_t)dat - 32][i];
                for (j = 0; j < 8; j++) {
                    buf[i + j * 6] = (tmp & 0x01) ? ips200_pencolor : ips200_bgcolor;
                    tmp >>= 1;
                }
            }
            ips200_write_16bit_data_array(buf, 6 * 8);
        } break;
        case IPS200_8X16_FONT: {
            uint16_t buf[8 * 16];
            ips200_set_region(x, y, x + 7, y + 15);
            for (i = 0; i < 8; i++) {
                uint8_t top    = ascii_font_8x16[(uint8_t)dat - 32][i];
                uint8_t bottom = ascii_font_8x16[(uint8_t)dat - 32][i + 8];
                for (j = 0; j < 8; j++) {
                    buf[i + j * 8]          = (top    & 0x01) ? ips200_pencolor : ips200_bgcolor;
                    buf[i + j * 8 + 4 * 16] = (bottom & 0x01) ? ips200_pencolor : ips200_bgcolor;
                    top    >>= 1;
                    bottom >>= 1;
                }
            }
            ips200_write_16bit_data_array(buf, 8 * 16);
        } break;
    }
}

void ips200_show_string(uint16_t x, uint16_t y, const char dat[])
{
    uint16_t j = 0;
    while (dat[j] != '\0') {
        switch (ips200_display_font) {
            case IPS200_6X8_FONT:  ips200_show_char(x + 6 * j, y, dat[j]); break;
            case IPS200_8X16_FONT: ips200_show_char(x + 8 * j, y, dat[j]); break;
        }
        j++;
    }
}

static void int_to_str(char *buf, int32_t val)
{
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[12]; int i = 0, neg = 0;
    if (val < 0) { neg = 1; val = -val; }
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    if (neg) tmp[i++] = '-';
    int len = i;
    for (int k = 0; k < len; k++) buf[k] = tmp[len - 1 - k];
    buf[len] = '\0';
}

static void uint_to_str(char *buf, uint32_t val)
{
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[11]; int i = 0;
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int len = i;
    for (int k = 0; k < len; k++) buf[k] = tmp[len - 1 - k];
    buf[len] = '\0';
}

void ips200_show_int(uint16_t x, uint16_t y, int32_t dat, uint8_t num)
{
    int32_t offset = 1;
    char buf[12];
    memset(buf, ' ', num + 1); buf[num + 1] = '\0';
    if (num < 10) { for (uint8_t n = num; n > 0; n--) offset *= 10; dat %= offset; }
    int_to_str(buf, dat);
    ips200_show_string(x, y, buf);
}

void ips200_show_uint(uint16_t x, uint16_t y, uint32_t dat, uint8_t num)
{
    uint32_t offset = 1;
    char buf[12];
    memset(buf, ' ', num); buf[num] = '\0';
    if (num < 10) { for (uint8_t n = num; n > 0; n--) offset *= 10; dat %= offset; }
    uint_to_str(buf, dat);
    ips200_show_string(x, y, buf);
}

void func_double_to_str (char *str, double number, uint8_t point_bit)
{
    int data_int = 0;                                                           // 整数部分
    int data_float = 0.0;                                                       // 小数部分
    int data_temp[12];                                                          // 整数字符缓冲
    int data_temp_point[9];                                                     // 小数字符缓冲
    uint8_t bit = point_bit;                                                      // 转换精度位数

    do
    {
        if(NULL == str)
        {
            break;
        }

        // 提取整数部分
        data_int = (int)number;                                                 // 直接强制转换为 int
        if(0 > number)                                                          // 判断源数据是正数还是负数
        {
            *str ++ = '-';
        }
        else if(0.0 == number)                                                  // 如果是个 0
        {
            *str ++ = '0';
            *str ++ = '.';
            *str = '0';
            break;
        }

        // 提取小数部分
        number = number - data_int;                                             // 减去整数部分即可
        while(bit --)
        {
            number = number * 10;                                               // 将需要的小数位数提取到整数部分
        }
        data_float = (int)number;                                               // 获取这部分数值

        // 整数部分转为字符串
        bit = 0;
        do
        {
            data_temp[bit ++] = data_int % 10;                                  // 将整数部分倒序写入字符缓冲区
            data_int /= 10;
        }while(0 != data_int);
        while(0 != bit)
        {
            *str ++ = ((data_temp[bit - 1] < 0 ? -data_temp[bit - 1] : data_temp[bit - 1]) + 0x30);
            bit --;
        }

        // 小数部分转为字符串
        if(point_bit != 0)
        {
            bit = 0;
            *str ++ = '.';
            if(0 == data_float)
                *str = '0';
            else
            {
                while(0 != point_bit)                                           // 判断有效位数
                {
                    data_temp_point[bit ++] = data_float % 10;                  // 倒序写入字符缓冲区
                    data_float /= 10;
                    point_bit --;
                }
                while(0 != bit)
                {
                    *str ++ = ((data_temp_point[bit - 1] < 0 ? -data_temp_point[bit - 1] : data_temp_point[bit - 1]) + 0x30);
                    bit --;
                }
            }
        }
    }while(0);
}

void ips200_show_float (uint16_t x, uint16_t y, const double dat, uint8_t num, uint8_t pointnum)
{

    double dat_temp = dat;
    double offset = 1.0;
    char data_buffer[17] = {0};
    memset(data_buffer, ' ', num+pointnum+2);

    // 用来计算余数显示 123 显示 2 位则应该显示 23
    for(; 0 < num; num --)
    {
        offset *= 10;
    }
    dat_temp = dat_temp - ((int)dat_temp / (int)offset) * offset;
    func_double_to_str(data_buffer, dat_temp, pointnum);
    ips200_show_string(x, y, data_buffer);
}

// ------------------------------------------------------------------ image
void ips200_show_binary_image(uint16_t x, uint16_t y, const uint8_t *image,
                              uint16_t width, uint16_t height,
                              uint16_t dis_width, uint16_t dis_height)
{
    uint16_t buf[dis_width];
    ips200_set_region(x, y, x + dis_width - 1, y + dis_height - 1);
    for (uint16_t j = 0; j < dis_height; j++) {
        const uint8_t *row = image + (uint32_t)j * height / dis_height * width / 8;
        for (uint16_t i = 0; i < dis_width; i++) {
            uint32_t wi = (uint32_t)i * width / dis_width;
            uint8_t tmp = row[wi / 8];
            buf[i] = (tmp << (wi % 8) & 0x80) ? RGB565_WHITE : RGB565_BLACK;
        }
        ips200_write_16bit_data_array(buf, dis_width);
    }
}

void ips200_show_gray_image(uint16_t x, uint16_t y, const uint8_t *image,
                            uint16_t width, uint16_t height,
                            uint16_t dis_width, uint16_t dis_height, uint8_t threshold)
{
    uint16_t buf[dis_width];
    ips200_set_region(x, y, x + dis_width - 1, y + dis_height - 1);
    for (uint16_t j = 0; j < dis_height; j++) {
        const uint8_t *row = image + (uint32_t)j * height / dis_height * width;
        for (uint16_t i = 0; i < dis_width; i++) {
            uint8_t v = row[(uint32_t)i * width / dis_width];
            if (threshold == 0) {
                uint16_t c = (uint16_t)((v >> 3) & 0x1F) << 11;
                c |= (uint16_t)((v >> 2) & 0x3F) << 5;
                c |= (uint16_t)((v >> 3) & 0x1F);
                buf[i] = c;
            } else {
                buf[i] = (v < threshold) ? RGB565_BLACK : RGB565_WHITE;
            }
        }
        ips200_write_16bit_data_array(buf, dis_width);
    }
}

void ips200_show_rgb565_image(uint16_t x, uint16_t y, const uint16_t *image,
                              uint16_t width, uint16_t height,
                              uint16_t dis_width, uint16_t dis_height, uint8_t color_mode)
{
    uint16_t buf[dis_width];
    ips200_set_region(x, y, x + dis_width - 1, y + dis_height - 1);
    for (uint16_t j = 0; j < dis_height; j++) {
        const uint16_t *row = image + (uint32_t)j * height / dis_height * width;
        for (uint16_t i = 0; i < dis_width; i++) {
            buf[i] = row[(uint32_t)i * width / dis_width];
        }
        if (color_mode) {
            ips200_write_8bit_data_array((const uint8_t *)buf, dis_width * 2);
        } else {
            ips200_write_16bit_data_array(buf, dis_width);
        }
    }
}

void ips200_show_wave(uint16_t x, uint16_t y, const uint16_t *wave,
                      uint16_t width, uint16_t value_max,
                      uint16_t dis_width, uint16_t dis_value_max)
{
    uint16_t buf[dis_width];
    ips200_set_region(x, y, x + dis_width - 1, y + dis_value_max - 1);
    for (uint16_t j = 0; j < dis_value_max; j++) {
        for (uint16_t i = 0; i < dis_width; i++) buf[i] = ips200_bgcolor;
        ips200_write_16bit_data_array(buf, dis_width);
    }
    for (uint16_t i = 0; i < dis_width; i++) {
        uint32_t wi = (uint32_t)i * width / dis_width;
        uint16_t vi = (uint16_t)((uint32_t)wave[wi] * (dis_value_max - 1) / value_max);
        ips200_draw_point(x + i, y + (dis_value_max - 1) - vi, ips200_pencolor);
    }
}

// ------------------------------------------------------------------ init
void ips200_init(void)
{
    ips200_set_dir(ips200_display_dir);
    ips200_set_color(ips200_pencolor, ips200_bgcolor);

    IPS200_BLK_HIGH();
    IPS200_RES_LOW();
    UTIL_IdleMs(5);
    IPS200_RES_HIGH();
    UTIL_IdleMs(120);

    ips200_write_command(0x11);
    UTIL_IdleMs(120);

    ips200_write_command(0x36);
    switch (ips200_display_dir) {
        case IPS200_PORTAIT:       ips200_write_8bit_data(0x00); break;
        case IPS200_PORTAIT_180:   ips200_write_8bit_data(0xC0); break;
        case IPS200_CROSSWISE:     ips200_write_8bit_data(0x70); break;
        case IPS200_CROSSWISE_180: ips200_write_8bit_data(0xA0); break;
    }

    ips200_write_command(0x3A); ips200_write_8bit_data(0x05);

    ips200_write_command(0xB2);
    ips200_write_8bit_data(0x0C); ips200_write_8bit_data(0x0C);
    ips200_write_8bit_data(0x00); ips200_write_8bit_data(0x33);
    ips200_write_8bit_data(0x33);

    ips200_write_command(0xB7); ips200_write_8bit_data(0x35);
    ips200_write_command(0xBB); ips200_write_8bit_data(0x29);
    ips200_write_command(0xC2); ips200_write_8bit_data(0x01);
    ips200_write_command(0xC3); ips200_write_8bit_data(0x19);
    ips200_write_command(0xC4); ips200_write_8bit_data(0x20);
    ips200_write_command(0xC5); ips200_write_8bit_data(0x1A);
    ips200_write_command(0xC6); ips200_write_8bit_data(0x0F);

    ips200_write_command(0xD0);
    ips200_write_8bit_data(0xA4); ips200_write_8bit_data(0xA1);

    ips200_write_command(0xE0);
    ips200_write_8bit_data(0xD0); ips200_write_8bit_data(0x08);
    ips200_write_8bit_data(0x0E); ips200_write_8bit_data(0x09);
    ips200_write_8bit_data(0x09); ips200_write_8bit_data(0x05);
    ips200_write_8bit_data(0x31); ips200_write_8bit_data(0x33);
    ips200_write_8bit_data(0x48); ips200_write_8bit_data(0x17);
    ips200_write_8bit_data(0x14); ips200_write_8bit_data(0x15);
    ips200_write_8bit_data(0x31); ips200_write_8bit_data(0x34);

    ips200_write_command(0xE1);
    ips200_write_8bit_data(0xD0); ips200_write_8bit_data(0x08);
    ips200_write_8bit_data(0x0E); ips200_write_8bit_data(0x09);
    ips200_write_8bit_data(0x09); ips200_write_8bit_data(0x15);
    ips200_write_8bit_data(0x31); ips200_write_8bit_data(0x33);
    ips200_write_8bit_data(0x48); ips200_write_8bit_data(0x17);
    ips200_write_8bit_data(0x14); ips200_write_8bit_data(0x15);
    ips200_write_8bit_data(0x31); ips200_write_8bit_data(0x34);

    ips200_write_command(0x21);
    ips200_write_command(0x29);

    ips200_clear();
}
