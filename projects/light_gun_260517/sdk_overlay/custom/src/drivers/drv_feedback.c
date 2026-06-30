#include "drivers/drv_feedback.h"
#include "drivers/drv_rumble.h"
#include <stdint.h>

static int g_opened;
static int g_ready;
static uint8_t g_level;

static int fb_open(void *ctx)
{
    (void)ctx;
    g_opened = 1;
    g_level = 0;
    g_ready = (drv_rumble_init() == 0) ? 1 : 0;
    return (g_ready != 0) ? 0 : -1;
}

static int fb_close(void *ctx)
{
    (void)ctx;
    drv_rumble_deinit();
    g_opened = 0;
    g_ready = 0;
    return 0;
}

static int fb_read(void *ctx, uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0) || (len < 1U)) {
        return -1;
    }
    buf[0] = g_level;
    *out_len = 1;
    return 0;
}

static int fb_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((buf == 0) || (len < 1U) || (out_len == 0)) {
        return -1;
    }
    g_level = buf[0];
    (void)drv_rumble_set_level(g_level);
    *out_len = 1;
    return 0;
}

static int fb_ioctl(void *ctx, uint32_t cmd, void *arg)
{
    (void)ctx;
    (void)cmd;
    (void)arg;
    return 0;
}

static const of_fops_t g_ops = {
    .open = fb_open,
    .close = fb_close,
    .read = fb_read,
    .write = fb_write,
    .ioctl = fb_ioctl,
};

static of_dev_t g_dev = {
    .name = "feedback",
    .ops = &g_ops,
    .priv = 0,
};

const of_dev_t *drv_feedback_get_dev(void)
{
    return &g_dev;
}

int drv_feedback_is_ready(void)
{
    return (g_opened != 0) && (g_ready != 0);
}
