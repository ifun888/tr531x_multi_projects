#include "drivers/drv_temp_sensor.h"
#include <stdint.h>

static int g_opened;
static int g_ready;
static int16_t g_temp_c10;

static int temp_open(void *ctx)
{
    (void)ctx;
    g_opened = 1;
    g_ready = 1;
    g_temp_c10 = 250;
    return 0;
}

static int temp_close(void *ctx)
{
    (void)ctx;
    g_opened = 0;
    g_ready = 0;
    return 0;
}

static int temp_read(void *ctx, uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0) || (len < 2U)) {
        return -1;
    }
    buf[0] = (uint8_t)(g_temp_c10 & 0xFF);
    buf[1] = (uint8_t)((g_temp_c10 >> 8) & 0xFF);
    *out_len = 2;
    return 0;
}

static int temp_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    (void)ctx;
    if (out_len != 0) {
        *out_len = len;
    }
    if ((buf != 0) && (len >= 2U)) {
        g_temp_c10 = (int16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
    }
    return 0;
}

static int temp_ioctl(void *ctx, uint32_t cmd, void *arg)
{
    (void)ctx;
    (void)cmd;
    (void)arg;
    return 0;
}

static const of_fops_t g_ops = {
    .open = temp_open,
    .close = temp_close,
    .read = temp_read,
    .write = temp_write,
    .ioctl = temp_ioctl,
};

static of_dev_t g_dev = {
    .name = "temp_sensor",
    .ops = &g_ops,
    .priv = 0,
};

const of_dev_t *drv_temp_sensor_get_dev(void)
{
    return &g_dev;
}

int drv_temp_sensor_is_ready(void)
{
    return (g_opened != 0) && (g_ready != 0);
}
