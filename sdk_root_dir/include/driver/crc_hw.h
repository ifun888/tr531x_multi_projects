/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 * Description: Provides Hardware CRC api
 */

#ifndef CRC_HW_H
#define CRC_HW_H

#include <stdint.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * @defgroup drivers_driver_crc CRC
 * @ingroup  drivers_driver
 * @{
 */

/**
 * @if Eng
 * @brief  Generates a 16-bit hardeware CRC value. The polynomial is as follows: x16 + x12 + x5 + 1 (0x1021).
 * @param  [in]  crc_start  The original initial value is 0. Piecewise calculation is used.
 *                          The previous calculation result is transferred each time.
 * @param  [in]  buf        Pointer to the checked buffer.
 * @param  [in]  length     Length of the checked buffer (unit: byte).
 * @return CRC value.
 * @else
 * @brief  硬件计算16位CRC校验值. 多项式: x16 + x12 + x5 + 1 (0x1021)。
 * @param  [in]  crc_start  初始值为0。分段计算时，前一段结算的结果作为后一段计算的输入。
 * @param  [in]  buf        指向待计算数据的指针，由调用校验。
 * @param  [in]  length     数据长度 (单位: 字节)。
 * @return CRC计算结果.
 * @endif
 */
uint16_t uapi_crc16_hw(uint16_t crc_start, const uint8_t *buf, uint32_t length);

/**
 * @if Eng
 * @brief  Generates a 32-bit hardeware CRC value. It complies with the IEEE 802.3 CRC-32 standard (0x04C11DB7).
 * @param  [in]  crc_start  The original initial value is 0. Piecewise calculation is used.
 *                          The previous calculation result is transferred each time.
 * @param  [in]  buf        Pointer to the checked buffer.
 * @param  [in]  length     Length of the checked buffer (unit: byte).
 * @return CRC value.
 * @else
 * @brief  硬件计算32位CRC校验值. 多项式符合IEEE 802.3 CRC-32标准（0x04C11DB7）。
 * @param  [in]  crc_start  初始值为0。分段计算时，前一段结算的结果作为后一段计算的输入。
 * @param  [in]  buf        指向待计算数据的指针，由调用校验。
 * @param  [in]  length     数据长度 (单位: 字节)。
 * @return CRC计算结果.
 * @endif
 */
uint32_t uapi_crc32_hw(uint32_t crc_start, const uint8_t *buf, uint32_t length);

/**
 * @if Eng
 * @brief  Generates a 32-bit hardeware CRC value without two's complement. It complies with the IEEE 802.3
 *         CRC-32 standard (0x04C11DB7).
 * @param  [in]  crc_start  The original initial value is 0. Piecewise calculation is used.
 *                          The previous calculation result is transferred each time.
 * @param  [in]  buf        Pointer to the checked buffer.
 * @param  [in]  length     Length of the checked buffer (unit: byte).
 * @return CRC value.
 * @else
 * @brief  硬件计算32位CRC校验值（无补码）. 多项式符合IEEE 802.3 CRC-32标准（0x04C11DB7）。
 * @param  [in]  crc_start  初始值为0。分段计算时，前一段结算的结果作为后一段计算的输入。
 * @param  [in]  buf        指向待计算数据的指针，由调用校验。
 * @param  [in]  length     数据长度 (单位: 字节)。
 * @return CRC计算结果.
 * @endif
 */
uint32_t uapi_crc32_hw_no_comp(uint32_t crc_start, const uint8_t *buf, uint32_t length);


/**
 * @}
 */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif
