/**
 * Copyright (c) Triductor 2024. All rights reserved.
 *
 * Description: SLEM error code.
 */

/**
 * @defgroup slem_error_code Error Code API
 * @ingroup  SLEM
 * @{
 */

#ifndef SLEM_ERRCODE_H
#define SLEM_ERRCODE_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**
 * @if Eng
 * @brief  SLEM error code.
 * @else
 * @brief  SLEM 错误码（非协议相关）。
 * @endif
 */
typedef enum {
    ERRCODE_SLEM_SUCCESS = 0,                              /*!< @if Eng error code of success.
                                                                @else   执行成功错误码。 @endif */
    ERRCODE_SLEM_FAIL,                                     /*!< @if Eng error code of configure fail.
                                                                @else   配置失败错误码。 @endif */
    ERRCODE_SLEM_POS_FAIL,                                 /*!< @if Eng error code of position fail.
                                                                @else   定位失败码。 @endif */
    ERRCODE_SLEM_RSSI_ABNORMAL,                            /*!< @if Eng error code of rssi abnormal.
                                                                @else   RSSI异常。 @endif */
    ERRCODE_SLEM_MARIX_INV_FAIL,                           /*!< @if Eng error code of matrix inverse fail.
                                                                @else   矩阵求逆失败错误码。 @endif */
    ERRCODE_SLEM_MEMCPY_FAIL,                              /*!< @if Eng error code of memcpy fail.
                                                                @else   拷贝失败错误码。 @endif */
    ERRCODE_SLEM_MALLOC_FAIL,                              /*!< @if Eng error code of malloc fail.
                                                                @else   内存申请失败错误码。 @endif */
    ERRCODE_SLEM_TOF_IQ_NOTMATCH,                          /*!< @if Eng The ToF distance is much greater than
                                                                        the IQ distance.
                                                                @else   ToF测距值远大于IQ测距值。 @endif */
    ERRCODE_SLEM_IQ_LOW_ENERGY,                            /*!< @if Eng The IQ energy is too low.
                                                                @else   IQ能量幅度过低，难以计算距离。 @endif */
} errcode_slem;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /* SLE_ERRCODE_H */
/**
 * @}
 */
