/**
 * Copyright (c) Triductor. 2022-2022. All rights reserved.
 *
 * Description: Provides V151 HAL pwm \n
 *
 * History: \n
 * 2022-09-16， Create file. \n
 */
#ifndef HAL_PWM_V151_H
#define HAL_PWM_V151_H

#include "hal_pwm.h"
#include "hal_pwm_v151_regs_op.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * @defgroup drivers_hal_pwm_v151 PWM V151
 * @ingroup  drivers_hal_pwm
 * @{
 */

/**
 * @if Eng
 * @brief  Initialize device for hal PWM.
 * @retval ERRCODE_SUCC   Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  HAL层PWM的初始化接口。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t hal_pwm_v151_init(void);

/**
 * @if Eng
 * @brief  Deinitialize device for hal PWM.
 * @retval ERRCODE_SUCC   Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  HAL层PWM的去初始化接口。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
void hal_pwm_v151_deinit(void);

 /**
 * @if Eng
 * @brief  Set the PWM low/high level time.
 * @param  [in]  id Low/high level time ID.
 * @param  [in]  channel PWM device.
 * @param  [in]  time  The value to set the low/high time of PWM.
 * @else
 * @brief  设置PWM高低电平时间。
 * @param  [in]  id 高低电平时间ID。
 * @param  [in]  channel PWM设备。
 * @param  [in]  time  用于设置PWM高低电平时间的值。
 * @endif
 */
void hal_pwm_v151_set_time(hal_pwm_set_time_id_t id, pwm_channel_t channel, uint32_t time);

 /**
 * @if Eng
 * @brief  Set the PWM repeat cycles.
 * @param  [in]  channel PWM device.
 * @param  [in]  cycles  The value to set repeat cycles.
 * @else
 * @brief  设置PWM重复周期。
 * @param  [in]  channel PWM设备。
 * @param  [in]  cycles  重复周期值。
 * @endif
 */
void hal_pwm_v151_set_cycles(pwm_channel_t channel, uint16_t cycles);

 /**
 * @if Eng
 * @brief  Set the PWM action.
 * @param  [in]  channel PWM device.
 * @param  [in]  action  PWM action.
 * @else
 * @brief  设置PWM动作模式。
 * @param  [in]  channel PWM设备。
 * @param  [in]  action  PWM动作模式。
 * @endif
 */
void hal_pwm_v151_set_action(uint8_t channel, pwm_action_t action);

 /**
 * @if Eng
 * @brief  Clear the PWM interrupt.
 * @param  [in]  channel PWM device.
 * @else
 * @brief  清除PWM中断。
 * @param  [in]  channel PWM设备。
 * @endif
 */
void hal_pwm_v151_intr_clear(pwm_channel_t channel);

 /**
 * @if Eng
 * @brief  Register a callback asociated with a PWM interrupt cause.
 * @param  [in]  channel PWM device.
 * @param  [in]  callback  The interrupt callback to register.
 * @else
 * @brief  注册与PWM中断原因关联的回调。
 * @param  [in]  channel PWM设备。
 * @param  [in]  callback  寄存器的中断回调。
 * @endif
 */
void hal_pwm_v151_register_callback(pwm_channel_t channel, hal_pwm_callback_t callback);

 /**
 * @if Eng
 * @brief  Unregister a previously registered callback for a PWM device.
 * @param  [in]  channel PWM device.
 * @else
 * @brief  为PWM设备注销先前注册的回调。
 * @param  [in]  channel PWM设备。
 * @endif
 */
void hal_pwm_v151_unregister_callback(pwm_channel_t channel);

 /**
 * @if Eng
 * @brief  Assigning channels to PWM groups, one group can have more than one channel,
 *         the same channel cannot be in different groups at the same time.
 * @param  [in]  group PWM group.
 * @param  [in]  channel_id PWM device.
 * @else
 * @brief  为PWM组分配通道，一个组可以有多个通道，同一个通道不能同时位于不同的组里。
 * @param  [in]  group   PWM组。
 * @param  [in]  channel_id PWM设备。
 * @endif
 */
void hal_pwm_v151_set_group(pwm_v151_group_t group, uint16_t channel_id);

#if defined(CONFIG_PWM_PRELOAD)
  /**
 * @if Eng
 * @brief  config pwm preload param.
 * @param  [in]  group PWM group.
 * @else
 * @brief  为PWM配置预加载参数。
 * @param  [in]  group   PWM组。
 * @endif
 */
void hal_pwm_config_preload(uint8_t group);
#endif /* CONFIG_PWM_PRELOAD */

/**
 * @if Eng
 * @brief  Handler of the PWM interrupt request.
 * @else
 * @brief  PWM中断处理函数。
 * @endif
 */
void hal_pwm_v151_irq_handler(void);

/**
 * @}
 */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif