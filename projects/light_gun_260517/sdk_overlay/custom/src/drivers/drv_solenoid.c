#include "drivers/drv_solenoid.h"

#include <stdint.h>
#include <string.h>

#include "common_def.h"
#include "gpio.h"
#include "osal_debug.h"
#include "osal_timer.h"
#include "pinctrl.h"
#include "soc_osal.h"

#ifndef OF_SOLENOID_GPIO
#ifdef CONFIG_LIGHT_GUN_260517_SOLENOID_GPIO
#define OF_SOLENOID_GPIO CONFIG_LIGHT_GUN_260517_SOLENOID_GPIO
#else
#define OF_SOLENOID_GPIO 6
#endif
#endif

#ifndef OF_SOLENOID_MAX_ON_MS
#ifdef CONFIG_LIGHT_GUN_260517_SOLENOID_MAX_ON_MS
#define OF_SOLENOID_MAX_ON_MS CONFIG_LIGHT_GUN_260517_SOLENOID_MAX_ON_MS
#else
#define OF_SOLENOID_MAX_ON_MS 25U
#endif
#endif

#ifndef OF_SOLENOID_DEFAULT_ON_MS
#ifdef CONFIG_LIGHT_GUN_260517_SOLENOID_DEFAULT_ON_MS
#define OF_SOLENOID_DEFAULT_ON_MS CONFIG_LIGHT_GUN_260517_SOLENOID_DEFAULT_ON_MS
#else
#define OF_SOLENOID_DEFAULT_ON_MS 18U
#endif
#endif

#ifndef OF_SOLENOID_DEFAULT_OFF_MS
#ifdef CONFIG_LIGHT_GUN_260517_SOLENOID_DEFAULT_OFF_MS
#define OF_SOLENOID_DEFAULT_OFF_MS CONFIG_LIGHT_GUN_260517_SOLENOID_DEFAULT_OFF_MS
#else
#define OF_SOLENOID_DEFAULT_OFF_MS 70U
#endif
#endif

typedef enum {
    SOL_STATE_IDLE = 0,
    SOL_STATE_ON,
    SOL_STATE_OFF_WAIT,
} sol_state_t;

typedef struct {
    osal_timer timer;
    uint8_t timer_inited;
    uint8_t ready;
    uint8_t pin;
    uint8_t shots_total;
    uint8_t shots_done;
    uint32_t on_ms;
    uint32_t off_ms;
    sol_state_t state;
} sol_ctx_t;

static sol_ctx_t g_sol;

static void sol_gpio_write(uint8_t high)
{
    (void)uapi_gpio_set_val((pin_t)g_sol.pin, high ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
}

static uint32_t sol_clamp_on_ms(uint32_t on_ms)
{
    if (on_ms == 0U) {
        return 1U;
    }
    if (on_ms > OF_SOLENOID_MAX_ON_MS) {
        return OF_SOLENOID_MAX_ON_MS;
    }
    return on_ms;
}

static void sol_arm_timer(uint32_t delay_ms)
{
    (void)osal_timer_stop(&g_sol.timer);
    (void)osal_timer_mod(&g_sol.timer, delay_ms);
}

static void sol_finish(void)
{
    sol_gpio_write(0U);
    g_sol.state = SOL_STATE_IDLE;
    g_sol.shots_total = 0U;
    g_sol.shots_done = 0U;
}

static void sol_start_one_shot(void)
{
    g_sol.shots_done++;
    g_sol.state = SOL_STATE_ON;
    sol_gpio_write(1U);
    sol_arm_timer(g_sol.on_ms);
}

static void sol_timer_cb(unsigned long arg)
{
    unused(arg);

    if (g_sol.ready == 0U) {
        return;
    }

    switch (g_sol.state) {
        case SOL_STATE_ON:
            sol_gpio_write(0U);
            if (g_sol.shots_done >= g_sol.shots_total) {
                sol_finish();
            } else {
                g_sol.state = SOL_STATE_OFF_WAIT;
                sol_arm_timer(g_sol.off_ms);
            }
            break;
        case SOL_STATE_OFF_WAIT:
            sol_start_one_shot();
            break;
        case SOL_STATE_IDLE:
        default:
            sol_finish();
            break;
    }
}

int drv_solenoid_init(void)
{
    if (g_sol.ready != 0U) {
        return 0;
    }

    (void)memset(&g_sol, 0, sizeof(g_sol));
    g_sol.pin = (uint8_t)OF_SOLENOID_GPIO;

    uapi_gpio_init();
    (void)uapi_pin_set_mode((pin_t)g_sol.pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_pull((pin_t)g_sol.pin, PIN_PULL_UP);
    if (uapi_gpio_set_dir((pin_t)g_sol.pin, GPIO_DIRECTION_OUTPUT) != ERRCODE_SUCC) {
        osal_printk("[drv_solenoid] gpio dir failed, pin=%u.\r\n", (unsigned int)g_sol.pin);
        return -1;
    }
    sol_gpio_write(0U);

    g_sol.timer.timer = NULL;
    g_sol.timer.handler = sol_timer_cb;
    g_sol.timer.data = 0;
    g_sol.timer.interval = 1U;
    if (osal_timer_init(&g_sol.timer) != OSAL_SUCCESS) {
        osal_printk("[drv_solenoid] timer init failed.\r\n");
        return -1;
    }

    g_sol.timer_inited = 1U;
    g_sol.ready = 1U;
    osal_printk("[drv_solenoid] init ok, gpio=%u default LOW.\r\n", (unsigned int)g_sol.pin);
    return 0;
}

void drv_solenoid_deinit(void)
{
    if (g_sol.timer_inited != 0U) {
        (void)osal_timer_stop(&g_sol.timer);
    }
    sol_finish();
    g_sol.ready = 0U;
}

int drv_solenoid_is_ready(void)
{
    return (g_sol.ready != 0U);
}

int drv_solenoid_fire_once(uint32_t on_ms)
{
    if (g_sol.ready == 0U) {
        return -1;
    }

    g_sol.on_ms = sol_clamp_on_ms(on_ms);
    g_sol.off_ms = OF_SOLENOID_DEFAULT_OFF_MS;
    g_sol.shots_total = 1U;
    g_sol.shots_done = 0U;
    sol_start_one_shot();
    return 0;
}

int drv_solenoid_fire_burst(uint8_t shots, uint32_t on_ms, uint32_t off_ms)
{
    if ((g_sol.ready == 0U) || (shots == 0U)) {
        return -1;
    }

    g_sol.on_ms = sol_clamp_on_ms(on_ms);
    g_sol.off_ms = (off_ms == 0U) ? OF_SOLENOID_DEFAULT_OFF_MS : off_ms;
    g_sol.shots_total = shots;
    g_sol.shots_done = 0U;
    sol_start_one_shot();
    return 0;
}

int drv_solenoid_fire_level(uint8_t level)
{
    uint32_t on_ms;

    if (level == 0U) {
        drv_solenoid_stop();
        return 0;
    }

    on_ms = 6U + (((uint32_t)level * (OF_SOLENOID_MAX_ON_MS - 6U)) / 255U);
    if (on_ms < OF_SOLENOID_DEFAULT_ON_MS) {
        on_ms = OF_SOLENOID_DEFAULT_ON_MS;
    }
    return drv_solenoid_fire_once(on_ms);
}

void drv_solenoid_stop(void)
{
    if (g_sol.timer_inited != 0U) {
        (void)osal_timer_stop(&g_sol.timer);
    }
    sol_finish();
}
