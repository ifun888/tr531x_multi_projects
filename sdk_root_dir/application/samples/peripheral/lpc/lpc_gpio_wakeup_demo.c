/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: Low power control sample Source. \n
 *
 * History: \n
 * 2024-06-20, Create file. \n
 */
#include "pinctrl.h"
#include "gpio.h"
#include "pm_dev.h"
#include "gpio.h"
#include "soc_osal.h"
#include "app_init.h"

#define LPC_GPIO_TASK_PRIO                     24
#define LPC_GPIO_TASK_STACK_SIZE               0x1000
#define LPC_WAKEUP_GPIO                        CONFIG_LPC_GPIO_WAKEUP_PIN

static uint32_t g_lpc_gpio_suspend = 0;
static uint32_t g_lpc_gpio_resume = 0;
static uint32_t g_lpc_gpio_isr = 0;

static errcode_t lpc_gpio_sample_suspend(uintptr_t arg)
{
    unused(arg);
    g_lpc_gpio_suspend++;
    return 0;
}

static errcode_t lpc_gpio_sample_reume(uintptr_t arg)
{
    unused(arg);
    g_lpc_gpio_resume++;
    return 0;
}

static pm_dev_ops_t g_lpc_gpio_ops = {
    .suspend = lpc_gpio_sample_suspend,
    .resume = lpc_gpio_sample_reume,
};

static void lpc_gpio_wakeup_callback(pin_t pin, uintptr_t param)
{
    unused(pin);
    unused(param);
    g_lpc_gpio_isr++;

    osal_printk("lpc gpio test count isr:%u, suspend:%u, resume:%u\r\n",
        g_lpc_gpio_isr, g_lpc_gpio_suspend, g_lpc_gpio_resume);
}

static int lpc_gpio_task(const char *arg)
{
    unused(arg);

    osal_printk("lpc_gpio_task gpio:%u\r\n", LPC_WAKEUP_GPIO);
#if defined(CONFIG_PINCTRL_SUPPORT_IE)
    uapi_pin_set_ie(LPC_WAKEUP_GPIO, PIN_IE_1);
#endif /* CONFIG_PINCTRL_SUPPORT_IE */
    /* 设置GPIO模式 */
    uapi_pin_set_mode(LPC_WAKEUP_GPIO, PIN_MODE_0);
    /* 注册进入低功耗supend和退出低功耗resume接口，可选 */
    uapi_pm_register_dev_ops(PM_DEV_GPIO, &g_lpc_gpio_ops);

    /* 设置为输入模式 */
    uapi_gpio_set_dir(LPC_WAKEUP_GPIO, GPIO_DIRECTION_INPUT);
    /* 中断下降沿唤醒，不需要处理函数可以使用uapi_gpio_set_isr_mode */
    uapi_gpio_register_isr_func(LPC_WAKEUP_GPIO, GPIO_INTERRUPT_FALLING_EDGE, lpc_gpio_wakeup_callback);

    return 0;
}

static void lpc_gpio_sample_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)lpc_gpio_task, 0,
        "lpc_gpio_sample_entry", LPC_GPIO_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, LPC_GPIO_TASK_PRIO);
    }
    osal_kthread_unlock();
}

/* Run the lpc_gpio_sample_entry. */
app_run(lpc_gpio_sample_entry);
