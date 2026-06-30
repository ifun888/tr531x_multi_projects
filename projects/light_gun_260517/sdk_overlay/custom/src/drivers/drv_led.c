#include "drivers/drv_led.h"
#include "gpio.h"
#include "pinctrl.h"
#include "common_def.h"
#include <stdint.h>

#ifndef OF_LED_PIN
#ifdef CONFIG_LIGHT_GUN_260517_LED_PIN
#define OF_LED_PIN CONFIG_LIGHT_GUN_260517_LED_PIN
#else
#define OF_LED_PIN 2
#endif
#endif

#ifndef OF_LED_ACTIVE_HIGH
#if defined(CONFIG_LIGHT_GUN_260517_LED_ACTIVE_HIGH)
#define OF_LED_ACTIVE_HIGH 1
#else
#define OF_LED_ACTIVE_HIGH 0
#endif
#endif

static int g_opened;
static int g_ready;
static uint8_t g_led_state;

static int led_apply(uint8_t v)
{
    gpio_level_t level = GPIO_LEVEL_LOW;
    if (((v != 0U) && (OF_LED_ACTIVE_HIGH != 0)) || ((v == 0U) && (OF_LED_ACTIVE_HIGH == 0))) {
        level = GPIO_LEVEL_HIGH;
    }
#if defined(CONFIG_BLINKY_PIN)
    return (uapi_gpio_set_val(CONFIG_BLINKY_PIN, level) == ERRCODE_SUCC) ? 0 : -1;
#else
    return (uapi_gpio_set_val((pin_t)OF_LED_PIN, level) == ERRCODE_SUCC) ? 0 : -1;
#endif
}

static int led_open(void *ctx)
{
    errcode_t ret = ERRCODE_SUCC;
    (void)ctx;
#if defined(CONFIG_BLINKY_PIN)
    (void)uapi_pin_set_mode(CONFIG_BLINKY_PIN, HAL_PIO_FUNC_GPIO);
    ret = uapi_gpio_set_dir(CONFIG_BLINKY_PIN, GPIO_DIRECTION_OUTPUT);
#else
    (void)uapi_pin_set_mode((pin_t)OF_LED_PIN, HAL_PIO_FUNC_GPIO);
    ret = uapi_gpio_set_dir((pin_t)OF_LED_PIN, GPIO_DIRECTION_OUTPUT);
#endif
    g_opened = 1;
    g_led_state = 0;
    g_ready = (ret == ERRCODE_SUCC);
    (void)led_apply(0);
    return 0;
}

static int led_close(void *ctx)
{
    (void)ctx;
    g_opened = 0;
    g_ready = 0;
    return 0;
}

static int led_read(void *ctx, uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0) || (len < 1U)) {
        return -1;
    }
    buf[0] = g_led_state;
    *out_len = 1;
    return 0;
}

static int led_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((buf == 0) || (len < 1U) || (out_len == 0)) {
        return -1;
    }
    g_led_state = (buf[0] != 0U) ? 1U : 0U;
    (void)led_apply(g_led_state);
    *out_len = 1;
    return 0;
}

static int led_ioctl(void *ctx, uint32_t cmd, void *arg)
{
    (void)ctx;
    (void)cmd;
    (void)arg;
    return 0;
}

static const of_fops_t g_ops = {
    .open = led_open,
    .close = led_close,
    .read = led_read,
    .write = led_write,
    .ioctl = led_ioctl,
};

static of_dev_t g_dev = {
    .name = "led",
    .ops = &g_ops,
    .priv = 0,
};

const of_dev_t *drv_led_get_dev(void)
{
    return &g_dev;
}

int drv_led_is_ready(void)
{
    return (g_opened != 0) && (g_ready != 0);
}
