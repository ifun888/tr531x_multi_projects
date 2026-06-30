#include "drivers/drv_rumble.h"

#include <stdint.h>
#include <string.h>

#include "common_def.h"
#include "gpio.h"
#include "osal_debug.h"
#include "osal_timer.h"
#include "pinctrl.h"
#include "pwm.h"
#include "soc_osal.h"

#ifndef OF_RUMBLE_PWM_CHANNEL
#ifdef CONFIG_LIGHT_GUN_260517_RUMBLE_PWM_CHANNEL
#define OF_RUMBLE_PWM_CHANNEL CONFIG_LIGHT_GUN_260517_RUMBLE_PWM_CHANNEL
#else
#define OF_RUMBLE_PWM_CHANNEL 1
#endif
#endif

#ifndef OF_RUMBLE_PWM_GROUP_ID
#ifdef CONFIG_LIGHT_GUN_260517_RUMBLE_PWM_GROUP_ID
#define OF_RUMBLE_PWM_GROUP_ID CONFIG_LIGHT_GUN_260517_RUMBLE_PWM_GROUP_ID
#else
#define OF_RUMBLE_PWM_GROUP_ID 0
#endif
#endif

#ifndef OF_RUMBLE_PWM_PIN
#ifdef CONFIG_LIGHT_GUN_260517_RUMBLE_PWM_PIN
#define OF_RUMBLE_PWM_PIN CONFIG_LIGHT_GUN_260517_RUMBLE_PWM_PIN
#else
#define OF_RUMBLE_PWM_PIN 2
#endif
#endif

#ifndef OF_RUMBLE_PWM_PIN_MODE
#ifdef CONFIG_LIGHT_GUN_260517_RUMBLE_PWM_PIN_MODE
#define OF_RUMBLE_PWM_PIN_MODE CONFIG_LIGHT_GUN_260517_RUMBLE_PWM_PIN_MODE
#else
#define OF_RUMBLE_PWM_PIN_MODE 41
#endif
#endif

#ifndef OF_RUMBLE_MAX_ACTIVE_MS
#ifdef CONFIG_LIGHT_GUN_260517_RUMBLE_MAX_ACTIVE_MS
#define OF_RUMBLE_MAX_ACTIVE_MS CONFIG_LIGHT_GUN_260517_RUMBLE_MAX_ACTIVE_MS
#else
#define OF_RUMBLE_MAX_ACTIVE_MS 300U
#endif
#endif

#ifndef OF_RUMBLE_DEFAULT_ACTIVE_MS
#ifdef CONFIG_LIGHT_GUN_260517_RUMBLE_DEFAULT_ACTIVE_MS
#define OF_RUMBLE_DEFAULT_ACTIVE_MS CONFIG_LIGHT_GUN_260517_RUMBLE_DEFAULT_ACTIVE_MS
#else
#define OF_RUMBLE_DEFAULT_ACTIVE_MS 120U
#endif
#endif

typedef struct {
    osal_timer timer;
    uint8_t timer_inited;
    uint8_t ready;
    uint8_t pwm_opened;
    uint8_t channel;
    uint8_t group_id;
    uint8_t pin;
    uint8_t last_level;
} rumble_ctx_t;

static rumble_ctx_t g_rumble;

static void rumble_gpio_force_low(void)
{
    uapi_gpio_init();
    (void)uapi_pin_set_mode((pin_t)g_rumble.pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_pull((pin_t)g_rumble.pin, PIN_PULL_DOWN);
    (void)uapi_gpio_set_dir((pin_t)g_rumble.pin, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_val((pin_t)g_rumble.pin, GPIO_LEVEL_LOW);
}

static void rumble_cleanup_stale_pwm_pins(void)
{
    pin_t pin;

    uapi_gpio_init();
    for (pin = S_MGPIO0; pin <= S_MGPIO31; pin++) {
        pin_mode_t current_mode = uapi_pin_get_mode(pin);
        if ((current_mode < (pin_mode_t)HAL_PIO_PWM0) || (current_mode > (pin_mode_t)HAL_PIO_PWM11)) {
            continue;
        }
        (void)uapi_pin_set_mode(pin, HAL_PIO_FUNC_GPIO);
        (void)uapi_pin_set_pull(pin, PIN_PULL_DOWN);
        (void)uapi_gpio_set_dir(pin, GPIO_DIRECTION_OUTPUT);
        (void)uapi_gpio_set_val(pin, GPIO_LEVEL_LOW);
    }
}

static uint32_t rumble_level_to_percent(uint8_t level)
{
    return (((uint32_t)level) * 100U) / 255U;
}

static uint32_t rumble_clamp_active_ms(uint32_t active_ms)
{
    if (active_ms == 0U) {
        return 1U;
    }
    if (active_ms > OF_RUMBLE_MAX_ACTIVE_MS) {
        return OF_RUMBLE_MAX_ACTIVE_MS;
    }
    return active_ms;
}

static int rumble_apply_level(uint8_t level)
{
    pwm_config_t cfg;
    uint8_t channel_id;
    uint32_t percent = rumble_level_to_percent(level);
    errcode_t ret;

    cfg.low_time = 100U - percent;
    cfg.high_time = percent;
    cfg.offset_time = 0U;
    cfg.cycles = 0xFFU;
    cfg.repeat = true;

    ret = uapi_pin_set_mode((pin_t)g_rumble.pin, (pin_mode_t)OF_RUMBLE_PWM_PIN_MODE);
    if (ret != ERRCODE_SUCC) {
        osal_printk("[drv_rumble] pin mode failed, pin=%u mode=%u ret=%d.\r\n",
            (unsigned int)g_rumble.pin, (unsigned int)OF_RUMBLE_PWM_PIN_MODE, (int)ret);
        return -1;
    }

    if (g_rumble.pwm_opened == 0U) {
        if (uapi_pwm_init() != ERRCODE_SUCC) {
            return -1;
        }
        if (uapi_pwm_open(g_rumble.channel, &cfg) != ERRCODE_SUCC) {
            return -1;
        }
        channel_id = g_rumble.channel;
        (void)uapi_pwm_set_group(g_rumble.group_id, &channel_id, 1U);
        (void)uapi_pwm_start_group(g_rumble.group_id);
        g_rumble.pwm_opened = 1U;
    } else {
        if (uapi_pwm_update_cfg(g_rumble.channel, &cfg) != ERRCODE_SUCC) {
            return -1;
        }
    }

    g_rumble.last_level = level;
    return 0;
}

static void rumble_timer_cb(unsigned long arg)
{
    unused(arg);
    drv_rumble_stop();
}

int drv_rumble_init(void)
{
    if (g_rumble.ready != 0U) {
        return 0;
    }

    (void)memset(&g_rumble, 0, sizeof(g_rumble));
    g_rumble.channel = (uint8_t)OF_RUMBLE_PWM_CHANNEL;
    g_rumble.group_id = (uint8_t)OF_RUMBLE_PWM_GROUP_ID;
    g_rumble.pin = (uint8_t)OF_RUMBLE_PWM_PIN;

    rumble_cleanup_stale_pwm_pins();
    rumble_gpio_force_low();

    g_rumble.timer.timer = NULL;
    g_rumble.timer.handler = rumble_timer_cb;
    g_rumble.timer.data = 0;
    g_rumble.timer.interval = 1U;
    if (osal_timer_init(&g_rumble.timer) != OSAL_SUCCESS) {
        osal_printk("[drv_rumble] timer init failed.\r\n");
        return -1;
    }

    g_rumble.timer_inited = 1U;
    g_rumble.ready = 1U;
    osal_printk("[drv_rumble] init ok, ch=%u group=%u pin=%u mode=%u.\r\n",
        (unsigned int)g_rumble.channel, (unsigned int)g_rumble.group_id,
        (unsigned int)g_rumble.pin, (unsigned int)OF_RUMBLE_PWM_PIN_MODE);
    return 0;
}

void drv_rumble_deinit(void)
{
    drv_rumble_stop();
    if (g_rumble.pwm_opened != 0U) {
        (void)uapi_pwm_close(g_rumble.channel);
        uapi_pwm_deinit();
        g_rumble.pwm_opened = 0U;
    }
    g_rumble.ready = 0U;
}

int drv_rumble_is_ready(void)
{
    return (g_rumble.ready != 0U);
}

int drv_rumble_set_level(uint8_t level)
{
    if (g_rumble.ready == 0U) {
        return -1;
    }

    if (level == 0U) {
        drv_rumble_stop();
        return 0;
    }

    if (rumble_apply_level(level) != 0) {
        return -1;
    }

    if (g_rumble.timer_inited != 0U) {
        (void)osal_timer_stop(&g_rumble.timer);
        (void)osal_timer_mod(&g_rumble.timer, OF_RUMBLE_DEFAULT_ACTIVE_MS);
    }
    return 0;
}

int drv_rumble_pulse(uint8_t level, uint32_t active_ms)
{
    if (g_rumble.ready == 0U) {
        return -1;
    }
    if (level == 0U) {
        drv_rumble_stop();
        return 0;
    }
    if (rumble_apply_level(level) != 0) {
        return -1;
    }
    if (g_rumble.timer_inited != 0U) {
        (void)osal_timer_stop(&g_rumble.timer);
        (void)osal_timer_mod(&g_rumble.timer, rumble_clamp_active_ms(active_ms));
    }
    return 0;
}

void drv_rumble_stop(void)
{
    if (g_rumble.timer_inited != 0U) {
        (void)osal_timer_stop(&g_rumble.timer);
    }
    rumble_gpio_force_low();
    g_rumble.last_level = 0U;
}
