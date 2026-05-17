/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: Provides MIPI_RFFE driver api
 */
#ifndef MIPI_RFFE_H
#define MIPI_RFFE_H

#include <stdint.h>
#include <stddef.h>
#include "errcode.h"
#include "platform_core.h"
#include "hal_mipi_rffe_v151.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * @if Eng
 * @brief  MIPI_RFEE transfer data structure.
 * @else
 * @brief  MIPI_RFFE传输结构体。
 * @endif
 */
typedef hal_mipi_rffe_xfer_data_info_t mipi_rffe_xfer_data_info_t;

/**
 * @if Eng
 * @brief  Initialize the MIPI_RFFE.
 * @param  [in]  bus The MIPI_RFFE bus. For details, see @ref mipi_bus_index_t.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  初始化 MIPI_RFFE。
 * @param  [in]  bus 指定的MIPI_RFFE编号，参考 @ref mipi_bus_index_t 。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_mipi_rffe_init(mipi_bus_index_t bus);

/**
 * @if Eng
 * @brief  Deinitialize the MIPI_RFFE.
 * @param  [in]  bus The MIPI_RFFE bus. For details, see @ref mipi_bus_index_t.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, For details, see @ref errcode_t.
 * @else
 * @brief  去初始化 MIPI_RFFE。
 * @param  [in]  bus 指定的MIPI_RFFE编号，参考 @ref mipi_bus_index_t 。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_mipi_rffe_deinit(mipi_bus_index_t bus);

/**
 * @if Eng
 * @brief  write slave regs.
 * @param  [in]  bus The MIPI_RFFE bus. For details, see @ref mipi_bus_index_t.
 * @param  [in]  xfer_data Transfer data. For details, see @ref hal_mipi_rffe_xfer_data_info_t
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, For details, see @ref errcode_t.
 * @else
 * @brief  写slave寄存器。
 * @param  [in]  bus 指定的MIPI_RFFE编号，参考 @ref mipi_bus_index_t 。
 * @param  [in]  xfer_data 传输数据。参考 @ref hal_mipi_rffe_xfer_data_info_t
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_mipi_rffe_write(mipi_bus_index_t bus, mipi_rffe_xfer_data_info_t *xfer_data);

/**
 * @if Eng
 * @brief  read slave regs.
 * @param  [in]  bus The MIPI_RFFE bus. For details, see @ref mipi_bus_index_t.
 * @param  [in]  xfer_data Transfer data. For details, see @ref hal_mipi_rffe_xfer_data_info_t
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, For details, see @ref errcode_t.
 * @else
 * @brief  读slave寄存器。
 * @param  [in]  bus 指定的MIPI_RFFE编号，参考 @ref mipi_bus_index_t 。
 * @param  [in]  xfer_data 传输数据。参考 @ref hal_mipi_rffe_xfer_data_info_t
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_mipi_rffe_read(mipi_bus_index_t bus, mipi_rffe_xfer_data_info_t *xfer_data);

/**
 * @}
 */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif
