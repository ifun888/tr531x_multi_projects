/**
 * Copyright (c) Triductor. 2022-2022. All rights reserved.
 *
 * Description: Provides pwm driver source \n
 *
 * History: \n
 * 2022-09-16， Create file. \n
 */

#include "common_def.h"
#include "osal_interrupt.h"
#if defined(CONFIG_PWM_SUPPORT_LPM)
#include "securec.h"
#endif
#include "hal_pwm.h"
#include "pwm.h"

#define PWM_CYCLES_MAX_NUMBER      32767  /* Max number of PWM cycles */

static bool g_pwm_inited = false;
static bool g_pwm_opened[CONFIG_PWM_CHANNEL_NUM] = { false };
#if defined(CONFIG_PWM_USING_V151)
static uint16_t g_pwm_group[CONFIG_PWM_GROUP_NUM] = { 0 };
#endif /* CONFIG_PWM_USING_v151 */

#if defined(CONFIG_PWM_SUPPORT_LPM)
typedef struct {
    pwm_config_t cfg;
    uint8_t channel;
} pwm_cfg_record_t;
static pwm_cfg_record_t g_pwm_cfg_record[CONFIG_PWM_CHANNEL_NUM] = { 0 };
static bool g_pwm_irq_register = false;
#endif

static errcode_t uapi_pwm_check(uint8_t channel)
{
    if (unlikely(channel >= CONFIG_PWM_CHANNEL_NUM)) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }

    if (unlikely(!g_pwm_inited)) {
        return ERRCODE_PWM_NOT_INIT;
    }

    if (unlikely(!g_pwm_opened[channel])) {
        return ERRCODE_PWM_NOT_OPEN;
    }

    return ERRCODE_SUCC;
}

#if defined(CONFIG_PWM_USING_V151)
static errcode_t uapi_pwm_group_check(uint8_t group)
{
    if (unlikely(group >= CONFIG_PWM_GROUP_NUM)) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }

    return ERRCODE_SUCC;
}

static errcode_t uapi_pwm_get_group(uint8_t* group, uint8_t channel)
{
    uint8_t index;
    for (index = 0; index < CONFIG_PWM_GROUP_NUM; index++) {
        if ((g_pwm_group[index] & (uint16_t)(1 << channel)) > 0) {
            *group = index;
            break;
        }
    }

    if (index == CONFIG_PWM_GROUP_NUM) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }
    return ERRCODE_SUCC;
}
#endif /* CONFIG_PWM_USING_v151 */

errcode_t uapi_pwm_init(void)
{
    if (g_pwm_inited) {
        return ERRCODE_SUCC;
    }

    pwm_port_clock_enable(true);

    errcode_t ret = hal_pwm_init();
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    g_pwm_inited = true;

    return ret;
}

void uapi_pwm_deinit(void)
{
    if (!g_pwm_inited) {
        return;
    }

    for (uint8_t channel_i = (uint8_t)PWM_0; channel_i < (uint8_t)CONFIG_PWM_CHANNEL_NUM; channel_i++) {
        if (g_pwm_opened[channel_i]) {
            (void)uapi_pwm_close(channel_i);
        }
    }

    hal_pwm_deinit();

    pwm_port_clock_enable(false);

    g_pwm_inited = false;
}

errcode_t uapi_pwm_open(uint8_t channel, const pwm_config_t *cfg)
{
    if (unlikely(channel >= CONFIG_PWM_CHANNEL_NUM)) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }

    if (unlikely(!g_pwm_inited)) {
        return ERRCODE_PWM_NOT_INIT;
    }
    if (unlikely(g_pwm_opened[channel])) {
        (void)uapi_pwm_close(channel);
    }

    if (unlikely(cfg->cycles > PWM_CYCLES_MAX_NUMBER)) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }
#if defined(CONFIG_PWM_USING_V151)
    if (pwm_port_param_check(cfg) != ERRCODE_SUCC) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }
#endif
    uint32_t id = osal_irq_lock();
    hal_pwm_register_callback((pwm_channel_t)channel, (hal_pwm_callback_t)uapi_pwm_isr);

    hal_pwm_set_time(PWM_SET_LOW_TIME, (pwm_channel_t)channel, cfg->low_time);
    hal_pwm_set_time(PWM_SET_HIGH_TIME, (pwm_channel_t)channel, cfg->high_time);

#if defined(CONFIG_PWM_USING_V151)
    hal_pwm_set_time(PWM_SET_OFFSET_TIME, (pwm_channel_t)channel, cfg->offset_time);
#endif /* CONFIG_PWM_USING_V151 */

    hal_pwm_set_cycles((pwm_channel_t)channel, cfg->cycles);

    if (cfg->repeat) {
        hal_pwm_set_action(channel, PWM_ACTION_CONTINUE_SET);
    } else {
        hal_pwm_set_action(channel, PWM_ACTION_CONTINUE_CLR);
    }
    osal_irq_restore(id);

#if defined(CONFIG_PWM_SUPPORT_LPM)
    (void)memcpy_s(&g_pwm_cfg_record[channel].cfg, sizeof(pwm_config_t), cfg, sizeof(pwm_config_t));
#endif /* CONFIG_PWM_SUPPORT_LPM */

    g_pwm_opened[channel] = true;
    return ERRCODE_SUCC;
}


errcode_t uapi_pwm_close(uint8_t channel)
{
    errcode_t ret;
    ret = uapi_pwm_check(channel);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
#if defined(CONFIG_PWM_USING_V151)
    uint8_t close_group = 0;
    ret = uapi_pwm_get_group(&close_group, channel);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
#endif /* CONFIG_PWM_USING_V151 */

    uint32_t id = osal_irq_lock();
#if defined(CONFIG_PWM_USING_V150)
    hal_pwm_set_action(channel, PWM_ACTION_STOP);
    hal_pwm_set_action(channel, PWM_ACTION_CONTINUE_CLR);
#else
    uint16_t pre_channel_id = (uint16_t)(1 << channel);
    uint16_t post_channel_id = g_pwm_group[close_group];
    hal_pwm_set_group((pwm_v151_group_t)close_group, pre_channel_id);
    hal_pwm_set_action(close_group, PWM_ACTION_STOP);
    hal_pwm_set_group((pwm_v151_group_t)close_group, post_channel_id);
    g_pwm_group[close_group] = post_channel_id;
#endif /* CONFIG_PWM_USING_V150 */
    osal_irq_restore(id);

    uapi_pwm_unregister_interrupt((pwm_channel_t)channel);

    g_pwm_opened[channel] = false;

    return ret;
}

errcode_t uapi_pwm_start(uint8_t channel)
{
    errcode_t ret = ERRCODE_SUCC;
    ret = uapi_pwm_check(channel);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
#if defined(CONFIG_PWM_USING_V151)
    uint8_t start_group = 0;
    ret = uapi_pwm_get_group(&start_group, channel);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
#endif /* CONFIG_PWM_USING_V151 */

    uint32_t id = osal_irq_lock();
#if defined(CONFIG_PWM_USING_V150)
    hal_pwm_set_action(channel, PWM_ACTION_START);
#else
    uint16_t pre_channel_id = (uint16_t)(1 << channel);
    uint16_t post_channel_id = g_pwm_group[start_group];
    hal_pwm_set_group((pwm_v151_group_t)start_group, pre_channel_id);
    hal_pwm_set_action(start_group, PWM_ACTION_START);
    hal_pwm_set_group((pwm_v151_group_t)start_group, post_channel_id);
    g_pwm_group[start_group] = post_channel_id;
#endif /* CONFIG_PWM_USING_V150 */
    osal_irq_restore(id);

    return ret;
}

uint32_t uapi_pwm_get_frequency(uint8_t channel)
{
    return pwm_port_get_clock_value((pwm_channel_t)channel);
}

#if defined(CONFIG_PWM_USING_V150)
errcode_t uapi_pwm_stop(uint8_t channel)
{
    errcode_t ret = uapi_pwm_check(channel);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    uint32_t id = osal_irq_lock();
    hal_pwm_set_action(channel, PWM_ACTION_STOP);
    osal_irq_restore(id);

    return ret;
}

errcode_t uapi_pwm_update_duty_ratio(uint8_t channel, uint32_t low_time, uint32_t high_time)
{
    errcode_t ret = uapi_pwm_check(channel);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    uint32_t id = osal_irq_lock();
    hal_pwm_set_time(PWM_SET_LOW_TIME, (pwm_channel_t)channel, low_time);
    hal_pwm_set_time(PWM_SET_HIGH_TIME, (pwm_channel_t)channel, high_time);
    hal_pwm_set_action(channel, PWM_ACTION_REFRESH);
    osal_irq_restore(id);

#if defined(CONFIG_PWM_SUPPORT_LPM)
    g_pwm_cfg_record[channel].cfg.low_time = low_time;
    g_pwm_cfg_record[channel].cfg.high_time = high_time;
#endif /* CONFIG_PWM_SUPPORT_LPM */

    return ret;
}
#endif /* CONFIG_PWM_USING_V150 */

errcode_t uapi_pwm_isr(uint8_t channel)
{
    if (unlikely(channel >= CONFIG_PWM_CHANNEL_NUM)) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }

    uint32_t id = osal_irq_lock();
    hal_pwm_intr_clear((pwm_channel_t)channel);
    osal_irq_restore(id);

    return ERRCODE_SUCC;
}

errcode_t uapi_pwm_register_interrupt(uint8_t channel, pwm_callback_t callback)
{
    errcode_t ret = uapi_pwm_check(channel);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    pwm_port_register_irq((pwm_channel_t)channel);

    hal_pwm_register_callback((pwm_channel_t)channel, (hal_pwm_callback_t)callback);

#if defined(CONFIG_PWM_SUPPORT_LPM)
    g_pwm_irq_register = true;
#endif /* CONFIG_PWM_SUPPORT_LPM */

    return ret;
}

errcode_t uapi_pwm_unregister_interrupt(uint8_t channel)
{
    errcode_t ret = uapi_pwm_check(channel);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    pwm_port_unregister_irq((pwm_channel_t)channel);

    hal_pwm_unregister_callback((pwm_channel_t)channel);

#if defined(CONFIG_PWM_SUPPORT_LPM)
    g_pwm_irq_register = false;
#endif /* CONFIG_PWM_SUPPORT_LPM */

    return ret;
}

#if defined(CONFIG_PWM_USING_V151)
errcode_t uapi_pwm_set_group(uint8_t group, const uint8_t *channel_set, uint32_t channel_set_len)
{
    uint16_t channel_id = 0;
    if ((group >= CONFIG_PWM_GROUP_NUM) || (channel_set == NULL) || (channel_set_len == 0)) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }

    for (uint32_t i = 0; i < channel_set_len; i++) {
        channel_id |= (1 << channel_set[i]);
    }

    for (uint32_t i = 0; i < CONFIG_PWM_GROUP_NUM; i++) {
        if ((g_pwm_group[i] & channel_id) > 0) {
            return ERRCODE_PWM_INVALID_PARAMETER;
        }
    }

    uint32_t id = osal_irq_lock();
    hal_pwm_set_group((pwm_v151_group_t)group, channel_id);
    g_pwm_group[group] = channel_id;
    osal_irq_restore(id);

    return ERRCODE_SUCC;
}

errcode_t uapi_pwm_clear_group(uint8_t group)
{
    if (group >= CONFIG_PWM_GROUP_NUM) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }

    uint32_t id = osal_irq_lock();
    hal_pwm_set_group((pwm_v151_group_t)group, 0);
    g_pwm_group[group] = 0;
    osal_irq_restore(id);

    return ERRCODE_SUCC;
}

errcode_t uapi_pwm_start_group(uint8_t group)
{
    errcode_t ret = uapi_pwm_group_check(group);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    uint32_t id = osal_irq_lock();
    hal_pwm_set_action(group, PWM_ACTION_START);
    osal_irq_restore(id);

    return ret;
}

errcode_t uapi_pwm_stop_group(uint8_t group)
{
    errcode_t ret = uapi_pwm_group_check(group);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    uint32_t id = osal_irq_lock();
    hal_pwm_set_action(group, PWM_ACTION_STOP);
    osal_irq_restore(id);

    return ret;
}

errcode_t uapi_pwm_update_cfg(uint8_t channel, const pwm_config_t *cfg)
{
    errcode_t ret = uapi_pwm_check(channel);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    if (unlikely(cfg->cycles > PWM_CYCLES_MAX_NUMBER)) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }

    if (pwm_port_param_check(cfg) != ERRCODE_SUCC) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }

    uint8_t start_group = 0;
    ret = uapi_pwm_get_group(&start_group, channel);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    uint32_t id = osal_irq_lock();
    hal_pwm_set_time(PWM_SET_LOW_TIME, (pwm_channel_t)channel, cfg->low_time);
    hal_pwm_set_time(PWM_SET_HIGH_TIME, (pwm_channel_t)channel, cfg->high_time);
    hal_pwm_set_time(PWM_SET_OFFSET_TIME, (pwm_channel_t)channel, cfg->offset_time);
    hal_pwm_set_cycles((pwm_channel_t)channel, cfg->cycles);
    hal_pwm_set_action(start_group, PWM_ACTION_REFRESH);
    osal_irq_restore(id);

#if defined(CONFIG_PWM_SUPPORT_LPM)
    (void)memcpy_s(&g_pwm_cfg_record[channel].cfg, sizeof(pwm_config_t), cfg, sizeof(pwm_config_t));
#endif /* CONFIG_PWM_SUPPORT_LPM */
    return ret;
}

#if defined(CONFIG_PWM_PRELOAD)
errcode_t uapi_pwm_config_preload(uint8_t group, uint8_t channel, const pwm_config_t *cfg)
{
    if ((channel >= CONFIG_PWM_CHANNEL_NUM) || (group >= CONFIG_PWM_GROUP_NUM) || (cfg == NULL)) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }

    if (unlikely(!g_pwm_inited)) {
        return ERRCODE_PWM_NOT_INIT;
    }

    if (unlikely(cfg->cycles > PWM_CYCLES_MAX_NUMBER)) {
        return ERRCODE_PWM_INVALID_PARAMETER;
    }

    uint32_t id = osal_irq_lock();
    hal_pwm_set_time(PWM_SET_LOW_TIME, (pwm_channel_t)channel, cfg->low_time);
    hal_pwm_set_time(PWM_SET_HIGH_TIME, (pwm_channel_t)channel, cfg->high_time);

    hal_pwm_set_time(PWM_SET_OFFSET_TIME, (pwm_channel_t)channel, cfg->offset_time);

    hal_pwm_set_cycles((pwm_channel_t)channel, cfg->cycles);
    hal_pwm_config_preload(group);
    osal_irq_restore(id);

#if defined(CONFIG_PWM_SUPPORT_LPM)
    (void)memcpy_s(&g_pwm_cfg_record[channel].cfg, sizeof(pwm_config_t), cfg, sizeof(pwm_config_t));
#endif /* CONFIG_PWM_SUPPORT_LPM */

    return ERRCODE_SUCC;
}
#endif /* CONFIG_PWM_PRELOAD */
#endif /* CONFIG_PWM_USING_V151 */

#if defined(CONFIG_PWM_SUPPORT_LPM)
errcode_t uapi_pwm_suspend(uintptr_t arg)
{
    unused(arg);
    return ERRCODE_SUCC;
}

errcode_t uapi_pwm_resume(uintptr_t arg)
{
    unused(arg);
    if (!g_pwm_inited) {
        return ERRCODE_SUCC;
    }
    g_pwm_inited = false;
    uapi_pwm_init();

    for (uint8_t channel = 0; channel < CONFIG_PWM_CHANNEL_NUM; channel++) {
        if (g_pwm_opened[channel]) {
            uapi_pwm_open(channel, &g_pwm_cfg_record[channel].cfg);
        }
    }
    if (g_pwm_irq_register) {
        pwm_port_register_irq(0);
    }
#if defined(CONFIG_PWM_USING_V151)
    for (uint32_t group = 0; group < CONFIG_PWM_GROUP_NUM; group++) {
        if (g_pwm_group[group] != 0) {
            hal_pwm_set_group((pwm_v151_group_t)group, g_pwm_group[group]);
        }
    }
#endif  /* CONFIG_PWM_USING_V151 */
    return ERRCODE_SUCC;
}
#endif  /* CONFIG_PWM_SUPPORT_LPM */
