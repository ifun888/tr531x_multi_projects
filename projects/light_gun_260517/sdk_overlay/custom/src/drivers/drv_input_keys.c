#include "drivers/drv_input_keys.h"

#include <stdint.h>
#include <string.h>

#include "common_def.h"
#include "gpio.h"
#include "pinctrl.h"
#include "platform/of_time.h"

#define OF_KEY_INVALID_PIN 255U

#ifndef OF_KEY_TRIGGER_PIN
#ifdef CONFIG_LIGHT_GUN_260517_KEY_TRIGGER_PIN
#define OF_KEY_TRIGGER_PIN CONFIG_LIGHT_GUN_260517_KEY_TRIGGER_PIN
#else
#define OF_KEY_TRIGGER_PIN 21
#endif
#endif

#ifndef OF_KEY_A_PIN
#ifdef CONFIG_LIGHT_GUN_260517_KEY_A_PIN
#define OF_KEY_A_PIN CONFIG_LIGHT_GUN_260517_KEY_A_PIN
#else
#define OF_KEY_A_PIN 23
#endif
#endif

#ifndef OF_KEY_B_PIN
#ifdef CONFIG_LIGHT_GUN_260517_KEY_B_PIN
#define OF_KEY_B_PIN CONFIG_LIGHT_GUN_260517_KEY_B_PIN
#else
#define OF_KEY_B_PIN 22
#endif
#endif

#ifndef OF_KEY_START_PIN
#ifdef CONFIG_LIGHT_GUN_260517_KEY_START_PIN
#define OF_KEY_START_PIN CONFIG_LIGHT_GUN_260517_KEY_START_PIN
#else
#define OF_KEY_START_PIN 18
#endif
#endif

#ifndef OF_KEY_SELECT_PIN
#ifdef CONFIG_LIGHT_GUN_260517_KEY_SELECT_PIN
#define OF_KEY_SELECT_PIN CONFIG_LIGHT_GUN_260517_KEY_SELECT_PIN
#else
#define OF_KEY_SELECT_PIN 17
#endif
#endif

#ifndef OF_KEY_HOME_PIN
#ifdef CONFIG_LIGHT_GUN_260517_KEY_HOME_PIN
#define OF_KEY_HOME_PIN CONFIG_LIGHT_GUN_260517_KEY_HOME_PIN
#else
#define OF_KEY_HOME_PIN 16
#endif
#endif

#ifndef OF_KEY_UP_PIN
#ifdef CONFIG_LIGHT_GUN_260517_KEY_UP_PIN
#define OF_KEY_UP_PIN CONFIG_LIGHT_GUN_260517_KEY_UP_PIN
#else
#define OF_KEY_UP_PIN 27
#endif
#endif

#ifndef OF_KEY_DOWN_PIN
#ifdef CONFIG_LIGHT_GUN_260517_KEY_DOWN_PIN
#define OF_KEY_DOWN_PIN CONFIG_LIGHT_GUN_260517_KEY_DOWN_PIN
#else
#define OF_KEY_DOWN_PIN 28
#endif
#endif

#ifndef OF_KEY_LEFT_PIN
#ifdef CONFIG_LIGHT_GUN_260517_KEY_LEFT_PIN
#define OF_KEY_LEFT_PIN CONFIG_LIGHT_GUN_260517_KEY_LEFT_PIN
#else
#define OF_KEY_LEFT_PIN 29
#endif
#endif

#ifndef OF_KEY_RIGHT_PIN
#ifdef CONFIG_LIGHT_GUN_260517_KEY_RIGHT_PIN
#define OF_KEY_RIGHT_PIN CONFIG_LIGHT_GUN_260517_KEY_RIGHT_PIN
#else
#define OF_KEY_RIGHT_PIN 30
#endif
#endif

#ifndef OF_KEY_MIDDLE_PIN
#ifdef CONFIG_LIGHT_GUN_260517_KEY_MIDDLE_PIN
#define OF_KEY_MIDDLE_PIN CONFIG_LIGHT_GUN_260517_KEY_MIDDLE_PIN
#else
#define OF_KEY_MIDDLE_PIN 0
#endif
#endif

#ifndef OF_KEY_DEBOUNCE_MS
#ifdef CONFIG_LIGHT_GUN_260517_KEY_DEBOUNCE_MS
#define OF_KEY_DEBOUNCE_MS CONFIG_LIGHT_GUN_260517_KEY_DEBOUNCE_MS
#else
#define OF_KEY_DEBOUNCE_MS 30U
#endif
#endif

typedef struct {
    pin_t pin;
    uint16_t mask;
    uint8_t enabled;
    uint8_t stable_pressed;
    uint8_t raw_pressed;
    uint32_t debounce_start_ms;
} of_key_item_t;

static int g_opened;
static int g_ready;
static uint16_t g_last_keys;

static of_key_item_t g_keys[] = {
    {(pin_t)OF_KEY_TRIGGER_PIN, OF_KEY_MASK_TRIGGER, 1, 0, 0, 0},
    {(pin_t)OF_KEY_A_PIN,       OF_KEY_MASK_A,       1, 0, 0, 0},
    {(pin_t)OF_KEY_B_PIN,       OF_KEY_MASK_B,       1, 0, 0, 0},
    {(pin_t)OF_KEY_START_PIN,   OF_KEY_MASK_START,   1, 0, 0, 0},
    {(pin_t)OF_KEY_SELECT_PIN,  OF_KEY_MASK_SELECT,  1, 0, 0, 0},
    {(pin_t)OF_KEY_HOME_PIN,    OF_KEY_MASK_HOME,    1, 0, 0, 0},
    {(pin_t)OF_KEY_UP_PIN,      OF_KEY_MASK_UP,      1, 0, 0, 0},
    {(pin_t)OF_KEY_DOWN_PIN,    OF_KEY_MASK_DOWN,    1, 0, 0, 0},
    {(pin_t)OF_KEY_LEFT_PIN,    OF_KEY_MASK_LEFT,    1, 0, 0, 0},
    {(pin_t)OF_KEY_RIGHT_PIN,   OF_KEY_MASK_RIGHT,   1, 0, 0, 0},
    {(pin_t)OF_KEY_MIDDLE_PIN,  OF_KEY_MASK_MIDDLE,  1, 0, 0, 0},
};

static uint32_t keys_now_ms(void)
{
    return (uint32_t)(of_time_us() / 1000U);
}

static void keys_refresh(void)
{
    uint32_t i;
    uint32_t now_ms = keys_now_ms();
    uint16_t mask = 0U;

    for (i = 0U; i < (sizeof(g_keys) / sizeof(g_keys[0])); i++) {
        gpio_level_t level;
        uint8_t pressed;

        if (g_keys[i].enabled == 0U || g_keys[i].pin == (pin_t)OF_KEY_INVALID_PIN) {
            continue;
        }

        level = uapi_gpio_get_val(g_keys[i].pin);
        pressed = (level == GPIO_LEVEL_LOW) ? 1U : 0U;
        if (pressed != g_keys[i].raw_pressed) {
            g_keys[i].raw_pressed = pressed;
            g_keys[i].debounce_start_ms = now_ms;
        } else if ((pressed != g_keys[i].stable_pressed) &&
            ((now_ms - g_keys[i].debounce_start_ms) >= OF_KEY_DEBOUNCE_MS)) {
            g_keys[i].stable_pressed = pressed;
        }

        if (g_keys[i].stable_pressed != 0U) {
            mask |= g_keys[i].mask;
        }
    }

    g_last_keys = mask;
}

static int keys_open(void *ctx)
{
    uint32_t i;
    errcode_t ret = ERRCODE_SUCC;

    (void)ctx;
    uapi_gpio_init();
    for (i = 0U; i < (sizeof(g_keys) / sizeof(g_keys[0])); i++) {
        if (g_keys[i].enabled == 0U || g_keys[i].pin == (pin_t)OF_KEY_INVALID_PIN) {
            continue;
        }
        (void)uapi_pin_set_mode(g_keys[i].pin, HAL_PIO_FUNC_GPIO);
        (void)uapi_pin_set_pull(g_keys[i].pin, PIN_PULL_UP);
        ret = uapi_gpio_set_dir(g_keys[i].pin, GPIO_DIRECTION_INPUT);
        if (ret != ERRCODE_SUCC) {
            g_ready = 0;
            return -1;
        }
    }

    g_opened = 1;
    g_ready = 1;
    g_last_keys = 0U;
    (void)memset(g_keys, 0, 0); /* keep compiler quiet about memset removal not needed */
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
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0U;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0) || (len < 2U)) {
        return -1;
    }

    keys_refresh();
    buf[0] = (uint8_t)(g_last_keys & 0xFFU);
    buf[1] = (uint8_t)((g_last_keys >> 8) & 0xFFU);
    *out_len = 2U;
    return 0;
}

static int keys_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    (void)ctx;
    if (out_len != 0) {
        *out_len = len;
    }
    if ((buf != 0) && (len >= 2U)) {
        g_last_keys = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    }
    return 0;
}

static int keys_ioctl(void *ctx, uint32_t cmd, void *arg)
{
    unused(ctx);
    unused(cmd);
    unused(arg);
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

uint16_t drv_input_keys_get_cached_mask(void)
{
    return g_last_keys;
}
