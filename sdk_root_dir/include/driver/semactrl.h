/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.. \n
 *
 * Description: Provides SEMA driver api \n
 */
#ifndef SEMACTRL_H
#define SEMACTRL_H

#include <stdint.h>
#include <stddef.h>
#include "errcode.h"
#include "sema_porting.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * @defgroup drivers_driver_sema Sema
 * @ingroup  drivers_driver
 * @{
 */

/**
 * @if Eng
 * @brief  Initialize the SEMA.
 * @else
 * @brief  初始化SEMA。
 * @endif
 */
void uapi_sema_init(void);

/**
 * @if Eng
 * @brief  Deinitialize the SEMA.
 * @else
 * @brief  去初始化SEMA。
 * @endif
 */
void uapi_sema_deinit(void);

/**
 * @if Eng
 * @brief  Set the sem_set param.
 * @param  [in] sema_index Index of Signal. see @ref sema_index_t.
 * @param  [in] wait_time_us Wait time len of Set the sub_clr param.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  获取信号量锁存。
 * @param  [in] sema_index 信号量索引，参考 @ref sema_index_t 。
 * @param  [in] wait_time_us 获取信号量锁存的超时等待时间。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_sema_get(uint8_t sema_index, uint32_t wait_time_us);

/**
 * @if Eng
 * @brief  Set the sub_clr param.
 * @param  [in] sema_index Index of Signal. see @ref sema_index_t.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  设置信号量释放锁存标记参数。
 * @param  [in] sema_index 信号量索引，参考 @ref sema_index_t 。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_sema_put(uint8_t sema_index);

/**
 * @if Eng
 * @brief  Set the force_clr param.
 * @param  [in] sema_index Index of Signal. see @ref sema_index_t.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  设置信号量强制清除锁存标记参数。
 * @param  [in] sema_index 信号量索引，参考 @ref sema_index_t 。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_sema_force_clear(uint8_t sema_index); // get/put接口锁中断接口在porting层实现，供持有信号量期间选择是否锁中断；同时加一下锁调度；

/**
 * @if Eng
 * @brief  Set the sub_clr param of all singals.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  设置所有信号量释放锁存标记参数。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_sema_clear_all(void);

/**
 * @}
 */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif