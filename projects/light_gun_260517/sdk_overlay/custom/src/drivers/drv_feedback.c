#include "drivers/drv_feedback.h"
#include "pwm.h"
#include "pinctrl.h"
#include "common_def.h"
#include <stdint.h>

#ifndef OF_PWM_CHANNEL
#define OF_PWM_CHANNEL 0
#endif
#ifndef OF_PWM_PIN
#define OF_PWM_PIN 0
#endif
#ifndef OF_PWM_PIN_MODE
#define OF_PWM_PIN_MODE 1
#endif

static int g_opened;
static int g_ready;
static uint8_t g_level;

static int fb_apply_level(uint8_t level)
{
    pwm_config_t cfg;
    cfg.low_time = (uint16_t)(100U - level);
    cfg.high_time = (uint16_t)level;
    cfg.offset_time = 0;
    cfg.cycles = 0xFF;
    cfg.repeat = true;
    if (uapi_pwm_update_cfg((uint8_t)OF_PWM_CHANNEL, &cfg) == ERRCODE_SUCC) {
        return 0;
    }
    return -1;
}

static int fb_open(void *ctx)
{
    pwm_config_t cfg;
    (void)ctx;
#if defined(CONFIG_PWM_PIN) && defined(CONFIG_PWM_PIN_MODE)
    (void)uapi_pin_set_mode(CONFIG_PWM_PIN, CONFIG_PWM_PIN_MODE);
#else
    (void)uapi_pin_set_mode((pin_t)OF_PWM_PIN, OF_PWM_PIN_MODE);
#endif
    cfg.low_time = 100;
    cfg.high_time = 0;
    cfg.offset_time = 0;
    cfg.cycles = 0xFF;
    cfg.repeat = true;
    g_opened = 1;
    g_level = 0;
    if (uapi_pwm_init() == ERRCODE_SUCC &&
        uapi_pwm_open((uint8_t)OF_PWM_CHANNEL, &cfg) == ERRCODE_SUCC &&
        uapi_pwm_start((uint8_t)OF_PWM_CHANNEL) == ERRCODE_SUCC) {
        g_ready = 1;
    } else {
        g_ready = 0;
    }
    return 0;
}

static int fb_close(void *ctx)
{
    (void)ctx;
    (void)uapi_pwm_close((uint8_t)OF_PWM_CHANNEL);
    (void)uapi_pwm_deinit();
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
    if (g_level > 100U) {
        g_level = 100U;
    }
    (void)fb_apply_level(g_level);
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
