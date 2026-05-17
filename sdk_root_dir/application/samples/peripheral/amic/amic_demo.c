/**
 * Copyright (c) Triductor. 2023-2024. All rights reserved.
 *
 * Description: AMIC Sample Source. \n
 *
 * History: \n
 * 2023-11-27, Create file. \n
 */
#include "gpio.h"
#include "pinctrl.h"
#include "adc.h"
#include "adc_porting.h"
#include "soc_osal.h"
#include "app_init.h"

#define AFE_GADC_CHANNEL7                 7
#define AFE_GADC_CHANNEL6                 6

#define ADC_TASK_PRIO                     24
#define ADC_TASK_STACK_SIZE               0x1000

static void app_afe_set_io(pin_t pin)
{
#if defined(CONFIG_PINCTRL_SUPPORT_IE)
    /* ADC管脚无需配置IE使能且管脚默认IE为0，为防止用户修改IE，特在此将IE配置为0 */
    uapi_pin_set_ie(pin, PIN_IE_0);
#endif
    uapi_pin_set_mode(pin, CONFIG_AFE_PIN_MODE);
    uapi_gpio_set_dir(pin, GPIO_DIRECTION_INPUT);
    uapi_pin_set_pull(pin, PIN_PULL_NONE);
}

static void *afe_task(const char *arg)
{
    unused(arg);
    uapi_pin_init();
    uapi_gpio_init();
    osal_printk("---start afe sample test start---\r\n");
    app_afe_set_io(CONFIG_AFE_USE_PIN1);
    app_afe_set_io(CONFIG_AFE_USE_PIN2);
    uapi_adc_init(ADC_CLOCK_NONE);
#ifdef CONFIG_ADC_SUPPORT_AMIC
    uapi_adc_power_en(AFE_AMIC_MODE, true);
#endif
#if defined(CONFIG_ADC_SUPPORT_DIFFERENTIAL)
    uapi_adc_open_differential_channel(AMIC_CHANNEL_0, AMIC_CHANNEL_1);
#endif
#ifdef CONFIG_ADC_SUPPORT_AMIC
    adc_calibration(AFE_AMIC_MODE, true, true, true);
#endif

    osal_printk("---start afe sample test end---\r\n");

    return NULL;
}

static void afe_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)afe_task, 0, "AFETask", ADC_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, ADC_TASK_PRIO);
    }
    osal_kthread_unlock();
}

/* Run the afe_entry. */
app_run(afe_entry);