/**
 * Copyright (c) Triductor. 2022. All rights reserved.
 *
 * Description: BTS Over the air module.
 */

/**
 * @defgroup bluetooth_bts_ota BTS LE OTA API
 * @ingroup  bluetooth
 * @{
 */
#ifndef BTS_LE_OTA_H
#define BTS_LE_OTA_H

#include <stdint.h>
#include <stdbool.h>
#include "errcode.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @if Eng
 * @brief Use this funtion to initializes bth ota channel.
 * @par   Use this funtion to initializes bth ota channel.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t
 * @par Dependency:
 * @li  bts_def.h
 * @else
 * @brief  初始化bth ota通道。
 * @par    初始化bth ota通道。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败。参考 @ref errcode_t
 * @par 依赖:
 * @li  bts_def.h
 * @endif
 */
errcode_t bth_ota_init(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif