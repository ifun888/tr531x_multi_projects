/**
 * Copyright (c) Triductor. 2022-2022. All rights reserved.
 *
 * Description: Provides HAL pwm \n
 *
 * History: \n
 * 2022-09-16， Create file. \n
 */

#include "common_def.h"
#include "hal_pwm.h"

uintptr_t g_pwm_regs = NULL;

errcode_t hal_pwm_regs_init(void)
{
    if (pwm_porting_base_addr_get() == 0) {
        return ERRCODE_PWM_REG_ADDR_INVALID;
    }

    g_pwm_regs = pwm_porting_base_addr_get();

    return ERRCODE_SUCC;
}

void hal_pwm_regs_deinit(void)
{
    g_pwm_regs = NULL;
}