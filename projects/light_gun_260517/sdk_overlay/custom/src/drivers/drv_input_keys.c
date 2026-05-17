#include "drivers/drv_input_keys.h"
#include "gpio.h"
#include "pinctrl.h"
#include "common_def.h"
#include <stdint.h>

#ifndef OF_KEY_PIN
#define OF_KEY_PIN 4
#endif

static int g_opened;
static int g_ready;
static uint8_t g_last_keys;

static int keys_open(void *ctx)
{
    errcode_t ret;
    (void)ctx;
#if defined(CONFIG_SAMPLE_BUTTON_PIN)
    (void)uapi_pin_set_mode(CONFIG_SAMPLE_BUTTON_PIN, HAL_PIO_FUNC_GPIO);
    ret = uapi_gpio_set_dir(CONFIG_SAMPLE_BUTTON_PIN, GPIO_DIRECTION_INPUT);
#else
    (void)uapi_pin_set_mode((pin_t)OF_KEY_PIN, HAL_PIO_FUNC_GPIO);
    ret = uapi_gpio_set_dir((pin_t)OF_KEY_PIN, GPIO_DIRECTION_INPUT);
#endif
    g_opened = 1;
    g_ready = (ret == ERRCODE_SUCC);
    g_last_keys = 0;
    return 0;
}

static int keys_close(void *ctx)
{
    (void)ctx;
    g_opened = 0;
    g_ready = 0;
    return 0;
}

static int keys_read(void *ctx, uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    gpio_level_t v;
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0) || (len < 1U)) {
        return -1;
    }
#if defined(CONFIG_SAMPLE_BUTTON_PIN)
    v = uapi_gpio_get_val(CONFIG_SAMPLE_BUTTON_PIN);
#else
    v = uapi_gpio_get_val((pin_t)OF_KEY_PIN);
#endif
    g_last_keys = (v == GPIO_LEVEL_LOW) ? 1U : 0U;
    buf[0] = g_last_keys;
    *out_len = 1;
    return 0;
}

static int keys_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    (void)ctx;
    if (out_len != 0) {
        *out_len = len;
    }
    if ((buf != 0) && (len > 0U)) {
        g_last_keys = buf[0];
    }
    return 0;
}

static int keys_ioctl(void *ctx, uint32_t cmd, void *arg)
{
    (void)ctx;
    (void)cmd;
    (void)arg;
    return 0;
}

static const of_fops_t g_ops = {
    .open = keys_open,
    .close = keys_close,
    .read = keys_read,
    .write = keys_write,
    .ioctl = keys_ioctl,
};

static of_dev_t g_dev = {
    .name = "input_keys",
    .ops = &g_ops,
    .priv = 0,
};

const of_dev_t *drv_input_keys_get_dev(void)
{
    return &g_dev;
}

int drv_input_keys_is_ready(void)
{
    return (g_opened != 0) && (g_ready != 0);
}
