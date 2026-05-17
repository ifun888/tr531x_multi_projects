#include "drivers/drv_input_adc.h"
#include "adc.h"
#include "common_def.h"
#include <stdint.h>

#ifndef OF_ADC_CH0
#define OF_ADC_CH0 0
#endif
#ifndef OF_ADC_CH1
#define OF_ADC_CH1 1
#endif

static int g_opened;
static int g_ready;
static uint16_t g_last_ch0;
static uint16_t g_last_ch1;

static int adc_open(void *ctx)
{
    (void)ctx;
    g_opened = 1;
    g_ready = (uapi_adc_init(ADC_CLOCK_NONE) == ERRCODE_SUCC);
    g_last_ch0 = 0;
    g_last_ch1 = 0;
    return 0;
}

static int adc_close(void *ctx)
{
    (void)ctx;
    (void)uapi_adc_deinit();
    g_opened = 0;
    g_ready = 0;
    return 0;
}

static int adc_read(void *ctx, uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    int32_t ch0;
    int32_t ch1;
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0) || (len < 4U)) {
        return -1;
    }

    ch0 = uapi_adc_auto_sample((uint8_t)OF_ADC_CH0);
    ch1 = uapi_adc_auto_sample((uint8_t)OF_ADC_CH1);
    if (ch0 >= 0) {
        g_last_ch0 = (uint16_t)ch0;
    }
    if (ch1 >= 0) {
        g_last_ch1 = (uint16_t)ch1;
    }

    buf[0] = (uint8_t)(g_last_ch0 & 0xFFU);
    buf[1] = (uint8_t)((g_last_ch0 >> 8) & 0xFFU);
    buf[2] = (uint8_t)(g_last_ch1 & 0xFFU);
    buf[3] = (uint8_t)((g_last_ch1 >> 8) & 0xFFU);
    *out_len = 4;
    g_ready = 1;
    return 0;
}

static int adc_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    (void)ctx;
    (void)buf;
    if (out_len != 0) {
        *out_len = len;
    }
    return 0;
}

static int adc_ioctl(void *ctx, uint32_t cmd, void *arg)
{
    (void)ctx;
    (void)cmd;
    (void)arg;
    return 0;
}

static const of_fops_t g_ops = {
    .open = adc_open,
    .close = adc_close,
    .read = adc_read,
    .write = adc_write,
    .ioctl = adc_ioctl,
};

static of_dev_t g_dev = {
    .name = "input_adc",
    .ops = &g_ops,
    .priv = 0,
};

const of_dev_t *drv_input_adc_get_dev(void)
{
    return &g_dev;
}

int drv_input_adc_is_ready(void)
{
    return (g_opened != 0) && (g_ready != 0);
}
