#include "ll_common.h"
#include "gpio.h"
#include "pinctrl.h"
#include "tcxo.h"

static uint64_t g_pulse_deadline_us;
static uint8_t g_pulse_active;

void ll_mark_init(pin_t pin)
{
    uapi_pin_set_mode(pin, HAL_PIO_FUNC_GPIO);
    uapi_gpio_set_dir(pin, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(pin, GPIO_LEVEL_LOW);
    g_pulse_deadline_us = 0;
    g_pulse_active = 0;
}

void ll_mark_pulse_start(pin_t pin)
{
    uapi_gpio_set_val(pin, GPIO_LEVEL_HIGH);
    g_pulse_deadline_us = uapi_tcxo_get_us() + LL_MARK_PULSE_US;
    g_pulse_active = 1;
}

void ll_mark_poll(pin_t pin)
{
    if ((g_pulse_active != 0U) && (uapi_tcxo_get_us() >= g_pulse_deadline_us)) {
        uapi_gpio_set_val(pin, GPIO_LEVEL_LOW);
        g_pulse_active = 0;
    }
}
