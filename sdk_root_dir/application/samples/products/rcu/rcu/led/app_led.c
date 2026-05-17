/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: APP LED. \n
 *
 * History: \n
 * 2024-05-23, Create file. \n
 */
#include "common_def.h"
#include "soc_osal.h"
#include "math.h"
#include "gpio.h"
#include "pinctrl.h"
#include "osal_timer.h"
#include "app_led.h"

#if defined (FLASH_1M)
uint8_t g_is_red_led_open = 0;

volatile uint8_t g_timer_led_flag = NO_LED;
volatile LED_STATUS g_led_status = LED_STATUS_IDLE;

osal_timer g_led_blink_timer;
osal_timer g_led_timeout_timer;

void red_led_open(void)
{
    g_is_red_led_open = 1;
    uapi_gpio_set_val(CONFIG_RED_LED_GPIO, LED_OPEN_GPIO_LEVEL);
}

void red_led_close(void)
{
    g_is_red_led_open = 0;
    uapi_gpio_set_val(CONFIG_RED_LED_GPIO, LED_CLOSE_GPIO_LEVEL);
}

void red_led_blink(void)
{
    if (g_is_red_led_open) {
        red_led_close();
    } else {
        red_led_open();
    }
}

void stop_led_timer(void)
{
    g_timer_led_flag = NO_LED;
    g_led_status = LED_STATUS_IDLE;
    osal_timer_stop(&g_led_timeout_timer);
}

void led_blink_timer_callback(unsigned long arg)
{
    unused(arg);

    switch (g_timer_led_flag) {
        case RED_LED:
            osal_timer_start(&g_led_blink_timer);
            red_led_blink();
            break;

        default:
            osal_timer_stop(&g_led_blink_timer);
            red_led_close();
            break;
    }
}

void led_timeout_timer_callback(unsigned long arg)
{
    unused(arg);
    stop_led_timer();
}

void start_led_timer(uint8_t color, uint32_t led_timer_period, uint32_t led_timer_timeout, LED_STATUS newstatus)
{
    if (g_led_status != LED_STATUS_IDLE && newstatus >= g_led_status) {
        return;
    }

    g_timer_led_flag = color;
    g_led_status = newstatus;

    switch (g_timer_led_flag) {
        case RED_LED:
            if (g_is_red_led_open) {
                red_led_close();
            } else {
                red_led_open();
            }
            break;
 
        default:
            break;
    }

    osal_timer_stop(&g_led_blink_timer);
    if (led_timer_period != 0) {
        if (!g_is_red_led_open) {
            red_led_blink();
        }
        g_led_blink_timer.data = (unsigned long)g_timer_led_flag;
        osal_timer_mod(&g_led_blink_timer, led_timer_period);
    }

    osal_timer_stop(&g_led_timeout_timer);
    if (led_timer_timeout != 0) {
        if (g_is_red_led_open) {
            osal_timer_mod(&g_led_timeout_timer, led_timer_timeout);
        } else {
            osal_timer_mod(&g_led_timeout_timer, led_timer_timeout + SLE_RCU_PAIR_LED_BLINK_TIME);
        }
    }
}

LED_STATUS get_led_status(void)
{
    return g_led_status;
}

void led_gpio_init(void)
{
    uapi_pin_set_mode(CONFIG_RED_LED_GPIO, HAL_PIO_FUNC_GPIO);     /* 设置指定IO复用为GPIO模式 */
    uapi_gpio_set_dir(CONFIG_RED_LED_GPIO, GPIO_DIRECTION_OUTPUT); /* 设置指定GPIO为输入模式 */
    uapi_gpio_set_val(CONFIG_RED_LED_GPIO, LED_CLOSE_GPIO_LEVEL);
    uapi_pin_set_mode(1, HAL_PIO_FUNC_GPIO);     /* 设置指定IO复用为GPIO模式 */
    uapi_gpio_set_dir(1, GPIO_DIRECTION_OUTPUT); /* 设置指定GPIO为输入模式 */
    uapi_gpio_set_val(1, LED_CLOSE_GPIO_LEVEL);
    g_led_status = LED_STATUS_IDLE;
}

void led_gpio_deinit(void)
{
    uapi_pin_set_mode(CONFIG_RED_LED_GPIO, HAL_PIO_FUNC_GPIO);    /* 设置指定IO复用为GPIO模式 */
    uapi_gpio_set_dir(CONFIG_RED_LED_GPIO, GPIO_DIRECTION_INPUT); /* 设置指定GPIO为输入模式 */
    uapi_pin_set_pull(CONFIG_RED_LED_GPIO, PIN_PULL_DOWN);
}

void app_led_init(void)
{
    led_gpio_init();
    g_led_blink_timer.timer = NULL;
    g_led_blink_timer.data = 0;
    g_led_blink_timer.handler = led_blink_timer_callback;
    g_led_blink_timer.interval = LED_TIMER_INIT_INTERVAL;
    uint32_t ret = osal_timer_init(&g_led_blink_timer);
    if (ret != OSAL_SUCCESS) {
        osal_printk("g_led_blink_timer create failed!\r\n");
    }

    g_led_timeout_timer.timer = NULL;
    g_led_timeout_timer.data = 0;
    g_led_timeout_timer.handler = led_timeout_timer_callback;
    g_led_timeout_timer.interval = LED_TIMER_INIT_INTERVAL;
    ret = osal_timer_init(&g_led_timeout_timer);
    if (ret != OSAL_SUCCESS) {
        osal_printk("g_led_timeout_timer create failed!\r\n");
    }
}

#else
void app_led_init(void)
{
}

void start_led_timer(uint8_t color, uint32_t led_timer_period, uint32_t led_timer_timeout, LED_STATUS newstatus)
{
    unused(color);
    unused(led_timer_period);
    unused(led_timer_timeout);
    unused(newstatus);
}

void stop_led_timer(void)
{
}
#endif
