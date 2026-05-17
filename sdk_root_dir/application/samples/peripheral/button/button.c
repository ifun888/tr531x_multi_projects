/**
 * Copyright (c) Triductor. 2023-2023. All rights reserved.
 *
 * Description: Button Sample Source. \n
 *
 * History: \n
 * 2023-04-03, Create file. \n
 */
#include "boards.h"
#include "pinctrl.h"
#include "gpio.h"
#include "osal_debug.h"
#include "app_init.h"
#include "tcxo.h"
#include "button.h"

#define BUTTON                  (pin_t)CONFIG_SAMPLE_BUTTON_PIN
#define DELAY_US                CONFIG_SAMPLE_BUTTON_DEBOUNCE_DELAY_US

static void gpio_callback_func(uint8_t pin_group, button_press_state_t state)
{
    unused(pin_group);
    unused(state);
    if (state == 0) {
        osal_printk("Button PRESSED\r\n");
    } else {
        osal_printk("Button RELEASED\r\n");
    }
}

static void button_entry(void)
{
    osal_printk("Button start.\r\n");
    uapi_button_init();
    uapi_button_send_msg_register(gpio_callback_func);
    uapi_button_gpio_register(BUTTON);
}

/* Run the button_entry. */
app_run(button_entry);