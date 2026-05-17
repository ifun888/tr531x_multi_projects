/**
 * Copyright (c) Triductor. 2022-2022. All rights reserved.
 *
 * Description: Provides hal pinctrl \n
 *
 * History: \n
 * 2022-08-29, Create file. \n
 */
#ifndef HAL_PINCTRL_TR531X_H
#define HAL_PINCTRL_TR531X_H

#include "hal_pinctrl.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * @defgroup drivers_hal_pinctrl_tr531x Pinctrl TR531X
 * @ingroup  drivers_hal_pinctrl
 * @{
 */

extern uint32_t g_pin_is_gpio;

/**
 * @brief Get configuration functions of pins.
 * @return Configuration functions of pins. see @ref hal_pin_funcs_t
 */
hal_pin_funcs_t *hal_pin_tr531x_funcs_get(void);

/**
 * @}
 */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif