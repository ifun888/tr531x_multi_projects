/**
 * Copyright (c) Triductor. 2022-2022. All rights reserved.
 *
 * Description: Provides V151 HAL pwm \n
 *
 * History: \n
 * 2022-09-16， Create file. \n
 */
#include "common_def.h"
#include "osal_debug.h"
#include "hal_pwm.h"
#include "tcxo.h"
#include "hal_pwm_v151.h"

#define PWM_INT_MASK_BIT 3
#define PWM_START_INTERVAL_US 1
#define PWM_LEN_HIGH_16BIT_RIGHT_SHIFT 16

static hal_pwm_callback_t g_hal_pwm_v151_callback[CONFIG_PWM_CHANNEL_NUM] = { NULL };
static uint32_t g_freq_time = 0;
static bool g_repeat_flag = 0;

#pragma weak hal_pwm_init = hal_pwm_v151_init
errcode_t hal_pwm_v151_init(void)
{
    if (hal_pwm_regs_init() != ERRCODE_SUCC) {
        return ERRCODE_PWM_REG_ADDR_INVALID;
    }

    /* Remove interrupt mask */
    hal_pwm_int_mask_set_pwm_int_mask(PWM_INT_MASK_BIT);
    return ERRCODE_SUCC;
}

#pragma weak hal_pwm_deinit = hal_pwm_v151_deinit
void hal_pwm_v151_deinit(void)
{
    hal_pwm_regs_deinit();
}

static void hal_pwm_v151_set_low_time(hal_pwm_set_time_id_t id, pwm_channel_t channel, uint32_t time)
{
    unused(id);
    unused(channel);
    g_freq_time = 0;
    g_freq_time += time;
}

static void hal_pwm_v151_set_high_time(hal_pwm_set_time_id_t id, pwm_channel_t channel, uint32_t time)
{
    unused(id);
    g_freq_time += time;

    hal_pwm_duty_l_set_pwm_duty_l_j(channel, time);
#ifdef CONFIG_PWM_SUPPORT_TIME_32BIT
    hal_pwm_duty_h_set_pwm_duty_h_j(channel, time >> PWM_LEN_HIGH_16BIT_RIGHT_SHIFT);
#endif /* CONFIG_PWM_SUPPORT_TIME_32BIT */
}

static void hal_pwm_v151_set_offset_time(hal_pwm_set_time_id_t id, pwm_channel_t channel, uint32_t time)
{
    unused(id);

    /* set offset time */
    hal_pwm_offset_l_set_pwm_offset_l_j(channel, time);

    /* set freq time */
    hal_pwm_freq_l_set_pwm_freq_l_j(channel, g_freq_time);
#ifdef CONFIG_PWM_SUPPORT_TIME_32BIT
    hal_pwm_offset_h_set_pwm_offset_h_j(channel, time >> PWM_LEN_HIGH_16BIT_RIGHT_SHIFT);
    hal_pwm_freq_h_set_pwm_freq_h_j(channel, g_freq_time >> PWM_LEN_HIGH_16BIT_RIGHT_SHIFT);
#endif /* CONFIG_PWM_SUPPORT_TIME_32BIT */
    g_freq_time = 0;
}

static hal_pwm_set_time_t g_hal_pwm_set_func_array[PWM_SET_TIEM_MAX] = {
    hal_pwm_v151_set_low_time,
    hal_pwm_v151_set_high_time,
    hal_pwm_v151_set_offset_time,
};

#pragma weak hal_pwm_set_time = hal_pwm_v151_set_time
void hal_pwm_v151_set_time(hal_pwm_set_time_id_t id, pwm_channel_t channel, uint32_t time)
{
    g_hal_pwm_set_func_array[id](id, channel, time);
}

#pragma weak hal_pwm_set_group = hal_pwm_v151_set_group
void hal_pwm_v151_set_group(pwm_v151_group_t group, uint16_t channel_id)
{
    /* Each bit in channel_id corresponds to one pwm channel */
    hal_pwm_sel_set_pwm_sel_i(group, channel_id);
}

static void hal_pwm_v151_set_group_en(pwm_v151_group_t group, bool en)
{
    uint16_t channel_id = hal_pwm_sel_get_pwm_sel_i(group);
    for (uint8_t i = 0; i < CONFIG_PWM_CHANNEL_NUM; i++) {
        if ((channel_id & (1 << i)) != 0) {
            hal_pwm_en_set_pwm_en_j(i, en);
        }
    }

    hal_pwm_startclrcnt_en_set_pwm_startclrcnt_en_i(group, 1);
    hal_pwm_start_set_pwm_start_i(group, 1);
#if !defined(CONFIG_PWM_PRELOAD)
    if (g_repeat_flag == false) {
        for (uint8_t i = 0; i < CONFIG_PWM_CHANNEL_NUM; i++) {
            if ((channel_id & (1 << i)) > 0 && hal_pwm_period_val_get_pwm_period_val_j(i) > 0) {
                hal_pwm_en_set_pwm_en_j(i, 0);
            }
        }
        uapi_tcxo_delay_us(PWM_START_INTERVAL_US);
        hal_pwm_startclrcnt_en_set_pwm_startclrcnt_en_i(group, 0);
        hal_pwm_start_set_pwm_start_i(group, 1);
    }
#else
    hal_pwm_startclrcnt_en_set_pwm_startclrcnt_en_i(group, 0);
#endif /* CONFIG_PWM_PRELOAD */
}

#if defined(CONFIG_PWM_PRELOAD)
void hal_pwm_config_preload(uint8_t group)
{
    hal_pwm_start_set_pwm_start_i(group, 1);
}
#endif /* CONFIG_PWM_PRELOAD */

static void hal_pwm_v151_start(pwm_v151_group_t group)
{
    hal_pwm_v151_set_group_en(group, true);
}

static void hal_pwm_v151_stop(pwm_v151_group_t group)
{
    hal_pwm_v151_set_group_en(group, false);
}

static void hal_pwm_v151_refresh(pwm_v151_group_t group)
{
    hal_pwm_startclrcnt_en_set_pwm_startclrcnt_en_i(group, 1);
    hal_pwm_start_set_pwm_start_i(group, 1);
}

#pragma weak hal_pwm_set_action = hal_pwm_v151_set_action
void hal_pwm_v151_set_action(uint8_t group, pwm_action_t action)
{
    if (action == PWM_ACTION_START) {
        hal_pwm_v151_start((pwm_v151_group_t)group);
    } else if (action == PWM_ACTION_STOP) {
        hal_pwm_v151_stop((pwm_v151_group_t)group);
    } else if (action == PWM_ACTION_CONTINUE_CLR) {
        g_repeat_flag = false;
    } else if (action == PWM_ACTION_CONTINUE_SET) {
        g_repeat_flag = true;
    } else if (action == PWM_ACTION_REFRESH) {
        hal_pwm_v151_refresh(group);
    }
}

#pragma weak hal_pwm_set_cycles = hal_pwm_v151_set_cycles
void hal_pwm_v151_set_cycles(pwm_channel_t channel, uint16_t cycles)
{
    hal_pwm_period_val_set_pwm_period_val_j(channel, cycles);
}

#pragma weak hal_pwm_intr_clear = hal_pwm_v151_intr_clear
void hal_pwm_v151_intr_clear(pwm_channel_t channel)
{
    uint32_t abnor_state0 = hal_pwm_abnor_state0_get_pwm_abnor_state0();
    uint32_t abnor_state1 = hal_pwm_abnor_state1_get_pwm_abnor_state1();
    uint32_t flag = hal_pwm_periodload_flag_get_pwm_periodload_flag_j(channel);

    hal_pwm_abnor_state_clr0_set_pwm_abnor_state_clr0(abnor_state0);
    hal_pwm_abnor_state_clr1_set_pwm_abnor_state_clr1(abnor_state1);

    if (flag != 0) {
        uint32_t cfg_int_clr0 = (bit((uint32_t)channel));
        hal_pwm_cfg_int_clr0_set_pwm_cfg_int_clr0(cfg_int_clr0);
    }
}

#pragma weak hal_pwm_register_callback = hal_pwm_v151_register_callback
void hal_pwm_v151_register_callback(pwm_channel_t channel, hal_pwm_callback_t callback)
{
    g_hal_pwm_v151_callback[channel] = callback;
}

#pragma weak hal_pwm_unregister_callback = hal_pwm_v151_unregister_callback
void hal_pwm_v151_unregister_callback(pwm_channel_t channel)
{
    g_hal_pwm_v151_callback[channel] = NULL;
}

void hal_pwm_v151_irq_handler(void)
{
    uint32_t abnor_state0 = hal_pwm_abnor_state0_get_pwm_abnor_state0();
    uint32_t abnor_state1 = hal_pwm_abnor_state1_get_pwm_abnor_state1();
    bool int_flag = false;
    for (uint8_t ch = 0; ch < CONFIG_PWM_CHANNEL_NUM; ch++) {
        int_flag = false;
        if ((((abnor_state0 >> ch) & 1) == 1)) {
            osal_printk("pwm %d group cfg err\r\n", ch);
            int_flag = true;
        }
        if ((((abnor_state1 >> ch) & 1) == 1)) {
            osal_printk("pwm %d time cfg err\r\n", ch);
            int_flag = true;
        }
        if (hal_pwm_periodload_flag_get_pwm_periodload_flag_j(ch) == 1) {
            if (g_hal_pwm_v151_callback[(pwm_channel_t)ch] != NULL) {
                g_hal_pwm_v151_callback[(pwm_channel_t)ch]((pwm_channel_t)ch);
            }
            int_flag = true;
        }
        if (int_flag) {
            hal_pwm_v151_intr_clear((pwm_channel_t)ch);
        }
    }
}
