/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 * Description: the header file of aes API. \n
 * \n
 * Create: 2024-06-11
*/

#ifndef SECURITY_SYMC_H
#define SECURITY_SYMC_H

#include "errcode.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**
 * @defgroup security_unified_symc Symc
 * @ingroup  drivers_driver_security_unified
 * @{
 */

/* SYMC(Single-Part). */
/**
 * @if Eng
 * @brief  Encrypt or Decrypt.
 * @param  [in]     alg Combine value. |<-1byte reserved->|<-1byte alg->|<-1byte work_mode->|<-1byte bit_width->|
 *                      Crypto alg, see @ref uapi_drv_cipher_symc_alg_t.
                        Crypto work mode， see @ref uapi_drv_cipher_symc_work_mode_t.
                        Crypto key bit width， see @ref uapi_drv_cipher_symc_bit_width_t.
 * @param  [in]     src Source data.
 * @param  [out]    dst Destination data.
 * @param  [in]     data_len Data length in byte.
 * @param  [inout]  iv The input is the initial vector, and the output is the updated vector.
 * @param  [in]     key Symmetric key.
 * @param  [in]     key_len Length of symmetric key, in byte.
 * @param  [in]     keyslot_handle Handle for storing the key. The handle is created and destroyed by the KM module.
 *                  If not need please set as INVALID_KEY_SLOT.
 * @param  [in]     is_encrypt Encrypted or not.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t
 * @note   There are two ways to set the key:
            1）Use the key/key_len combination. In this case, keyslot_handle is set to INVALID_KEY_SLOT。.
            2）Use the keyslot_handle/key_len combination to pass in the keyslot handle created
                by the keyslot interface. In this case, the key is NULL.
 * @else
 * @brief  加解密。
 * @param  [in]     alg 组合值。 |<-1byte reserved->|<-1byte alg->|<-1byte work_mode->|<-1byte bit_width->|
                        加密算法，见 @ref uapi_drv_cipher_symc_alg_t 。
                        工作模式， 参考 @ref uapi_drv_cipher_symc_work_mode_t 。
                        密钥位宽， 参考 @ref uapi_drv_cipher_symc_bit_width_t 。
 * @param  [in]     src 源数据。
 * @param  [out]    dst 目的数据。
 * @param  [in]     data_len 数据长度，单位是 Byte。
 * @param  [inout]  iv 输入为初始向量，输出为更新后的向量。
 * @param  [in]     key 对称密钥。
 * @param  [in]     key_len 对称密钥长度，单位是 Byte。
 * @param  [in]     keyslot_handle 用于保存key的句柄，通过 km 模块创建和销毁。
 *                  如果不需要，请设置为 INVALID_KEY_SLOT。
 * @param  [in]     is_encrypt 是否加密。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @note   有两种方法设置密钥:
            1）使用 key/key_len 组合，在这种情况下，keyslot_handle 传入 INVALID_KEY_SLOT。.
            2）使用 keyslot_handle/key_len 组合，keyslot 接口创建 keyslot 句柄传入，在这种情况下，key 传入为 NULL.
 * @endif
 */
errcode_t uapi_drv_cipher_symc_crypt(uint32_t alg, const uint8_t *src, uint8_t *dst, uint32_t data_len,
    uint8_t iv[16], const uint8_t *key, uint32_t key_len, uint32_t keyslot_handle, bool is_encrypt
);

/* SYMC(Multi-Part). */
/**
 * @if Eng
 * @brief  Initialize the context for multi-part encryption or decryption.
 * @param  [in]   symc_handle Symc handle.
 * @param  [in]   alg Combine value. |<-1byte reserved->|<-1byte alg->|<-1byte work_mode->|<-1byte bit_width->|
 *                     Crypto alg, see @ref uapi_drv_cipher_symc_alg_t.
                       Crypto work mode， see @ref uapi_drv_cipher_symc_work_mode_t.
                       Crypto key bit width， see @ref uapi_drv_cipher_symc_bit_width_t.
 * @param  [in]   key Symmetric key.
 * @param  [in]   key_len Length of symmetric key, in byte.
 * @param  [in]   keyslot_handle Handle for storing the key. The handle is created and destroyed by the KM module.
 *                 If not need please set as INVALID_KEY_SLOT.
 * @param  [in]   is_encrypt Encrypted or not.
 * @retval ERRCODE_SUCC Success.
 * @retval Other         Failure. For details, see @ref errcode_t
 * @note   There are two ways to set the key:
            1）Use the key/key_len combination. In this case, keyslot_handle is set to INVALID_KEY_SLOT.
            2）Use the keyslot_handle/key_len combination to pass in the keyslot handle created
                by the keyslot interface. In this case, the key is NULL.
 * @else
 * @brief  初始化分段加解密的上下文句柄。
 * @param  [in]   symc_handle Symc 句柄。
 * @param  [in]   alg 组合值。 |<-1byte reserved->|<-1byte alg->|<-1byte work_mode->|<-1byte bit_width->|
                     加密算法，见 @ref uapi_drv_cipher_symc_alg_t 。
                     工作模式， 参考 @ref uapi_drv_cipher_symc_work_mode_t 。
                     密钥位宽， 参考 @ref uapi_drv_cipher_symc_bit_width_t 。
 * @param  [in]   key 对称密钥。
 * @param  [in]   key_len 对称密钥长度，单位是 Byte。
 * @param  [in]   keyslot_handle 用于保存key的句柄，通过 km 模块创建和销毁。
 *                 如果不需要，请设置为 INVALID_KEY_SLOT。
 * @param  [in]   is_encrypt 是否加密。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other         失败，参考 @ref errcode_t 。
 * @note   有两种方法设置密钥:
            1）使用 key/key_len 组合，在这种情况下，keyslot_handle 传入 INVALID_KEY_SLOT。
            2）使用 keyslot_handle/key_len 组合，keyslot 接口创建 keyslot 句柄传入，在这种情况下，key 传入为 NULL。
 * @endif
 */
errcode_t uapi_drv_cipher_symc_setup(uint32_t *symc_handle, uint32_t alg,
    const uint8_t *key, uint32_t key_len, uint32_t *keyslot_handle, bool is_encrypt);

/**
 * @if Eng
 * @brief  Multi-part encryption or decryption for ECB mode.
 * @param  [in]   symc_handle Symc handle.
 * @param  [in]   src Source data.
 * @param  [out]  dst Destination data.
 * @param  [in]   data_len Data length in byte.
 * @retval ERRCODE_SUCC Success.
 * @retval Other         Failure. For details, see @ref errcode_t
 * @else
 * @brief  ECB模式下的分段加解密。
 * @param  [in]   symc_handle Symc 句柄。
 * @param  [in]   src 源数据。
 * @param  [out]  dst 目的数据。
 * @param  [in]   data_len 数据长度，单位是 Byte。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other         失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_symc_ecb_update(uint32_t symc_handle, const uint8_t *src, uint8_t *dst, uint32_t data_len);

/**
 * @if Eng
 * @brief  Multi-part encryption or decryption for CBC mode.
 * @param  [in]   symc_handle Symc handle.
 * @param  [in]   src Source data.
 * @param  [out]  dst Destination data.
 * @param  [in]   data_len Data length in byte.
 * @param  [inout] iv The input is the initial vector, and the output is the updated vector.
 * @retval ERRCODE_SUCC Success.
 * @retval Other         Failure. For details, see @ref errcode_t
 * @else
 * @brief  CBC 模式下的分段加解密。
 * @param  [in]   symc_handle Symc 句柄。
 * @param  [in]   src 源数据。
 * @param  [out]  dst 目的数据。
 * @param  [in]   data_len 数据长度，单位是 Byte。
 * @param  [inout] iv 输入为初始向量，输出为更新后的向量。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other         失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_symc_cbc_update(uint32_t symc_handle, const uint8_t *src, uint8_t *dst, uint32_t data_len,
    uint8_t iv[16]);

/**
 * @if Eng
 * @brief  Multi-part encryption or decryption for CTR mode.
 * @param  [in]   symc_handle Symc handle.
 * @param  [in]   src Source data.
 * @param  [out]  dst Destination data.
 * @param  [in]   data_len Data length in byte.
 * @param  [inout] nonce_counter The input is the initial 128-bit nonce and counter, and the output is the updated
                    128-bit nonce and counter.
 * @param  [inout] nc_off The offset in the current stream_block, for resuming within the current cipher stream.
                    It's value should be 0 at the start of the stream, and the output is the updated offset, it will be
                    maintained by the function.
 * @param  [out]  stream_block The output is the updated current stream_block.
 * @retval ERRCODE_SUCC Success.
 * @retval Other         Failure. For details, see @ref errcode_t
 * @else
 * @brief  CTR 模式下的分段加解密。
 * @param  [in]   symc_handle Symc 句柄。
 * @param  [in]   src 源数据。
 * @param  [out]  dst 目的数据。
 * @param  [in]   data_len 数据长度，单位是 Byte。
 * @param  [inout] nonce_counter 输入为初始 128 位 nonce 和 counter，输出为更新后的 128 位 nonce 和 counter。
 * @param  [inout] nc_off 在当前 stream block 中的偏移量，用于在当前加密流中恢复计算。
                        它的值应该在流的开始时为 0，输出为更新的偏移量，函数将维护它。
 * @param  [out]  stream_block 输出为更新后的当前 stream_block。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other         失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_symc_ctr_update(uint32_t symc_handle, const uint8_t *src, uint8_t *dst, uint32_t data_len,
    uint8_t nonce_counter[16], uint8_t *nc_off, uint8_t stream_block[16]);

/**
 * @if Eng
 * @brief  Multi-part encryption or decryption for OFB mode.
 * @param  [in]   symc_handle Symc handle.
 * @param  [in]   src Source data.
 * @param  [out]  dst Destination data.
 * @param  [in]   data_len Data length in byte.
 * @param  [inout] iv The input is the initial vector, and the output is the updated vector.
 * @param  [inout] iv_off The offset in the current iv, for resuming within the current cipher stream.
 * @retval ERRCODE_SUCC Success.
 * @retval Other         Failure. For details, see @ref errcode_t
 * @else
 * @brief  OFB 模式下的分段加解密。
 * @param  [in]   symc_handle Symc 句柄。
 * @param  [in]   src 源数据。
 * @param  [out]  dst 目的数据。
 * @param  [in]   data_len 数据长度，单位是 Byte。
 * @param  [inout] iv 输入为初始向量，输出为更新后的向量。
 * @param  [inout] iv_off 在当前 iv 中的偏移量，用于在当前加密流中恢复计算。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other         失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_symc_ofb_update(uint32_t symc_handle, const uint8_t *src, uint8_t *dst, uint32_t data_len,
    uint8_t iv[16], uint8_t *iv_off);

/**
 * @if Eng
 * @brief  Multi-part encryption or decryption for CFB8 mode.
 * @param  [in]   symc_handle Symc handle.
 * @param  [in]   src Source data.
 * @param  [out]  dst Destination data.
 * @param  [in]   data_len Data length in byte.
 * @param  [inout] iv The input is the initial vector, and the output is the updated vector.
 * @retval ERRCODE_SUCC Success.
 * @retval Other         Failure. For details, see @ref errcode_t
 * @else
 * @brief  CFB8 模式下的分段加解密。
 * @param  [in]   symc_handle Symc 句柄。
 * @param  [in]   src 源数据。
 * @param  [out]  dst 目的数据。
 * @param  [in]   data_len 数据长度，单位是 Byte。
 * @param  [inout] iv 输入为初始向量，输出为更新后的向量。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other         失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_symc_cfb8_update(uint32_t symc_handle, const uint8_t *src, uint8_t *dst, uint32_t data_len,
    uint8_t iv[16]);

/**
 * @if Eng
 * @brief  Multi-part encryption or decryption for CFB128 mode.
 * @param  [in]   symc_handle Symc handle.
 * @param  [in]   src Source data.
 * @param  [out]  dst Destination data.
 * @param  [in]   data_len Data length in byte.
 * @param  [inout] iv The input is the initial vector, and the output is the updated vector.
 * @param  [inout] iv_off The offset in the current iv, for resuming within the current cipher stream.
 * @retval ERRCODE_SUCC Success.
 * @retval Other         Failure. For details, see @ref errcode_t
 * @else
 * @brief  CFB128 模式下的分段加解密。
 * @param  [in]   symc_handle Symc 句柄。
 * @param  [in]   src 源数据。
 * @param  [out]  dst 目的数据。
 * @param  [in]   data_len 数据长度，单位是 Byte。
 * @param  [inout] iv 输入为初始向量，输出为更新后的向量。
 * @param  [inout] iv_off 在当前 iv 中的偏移量，用于在当前加密流中恢复计算。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other         失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_symc_cfb128_update(uint32_t symc_handle, const uint8_t *src, uint8_t *dst, uint32_t data_len,
    uint8_t iv[16], uint8_t *iv_off);

/**
 * @if Eng
 * @brief  Destroy the handle.
 * @param  [in]   symc_handle The symc handle to be destroyed.
 * @param  [in]   keyslot_handle The keyslot handle to be destroyed. If the handle is created by the keyslot interface,
 *                              you should pass INVALID_KEY_SLOT and destroy it by the keyslot interface. Else, you
 *                              should pass the keyslot handle created by the setup interface.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t
 * @else
 * @brief  销毁句柄。
 * @param  [in]   symc_handle 要销毁的 symc 句柄。
 * @param  [in]   keyslot_handle 要销毁的 keyslot 句柄。 如果句柄是通过 keyslot 接口创建的，
 *                              应该传入 INVALID_KEY_SLOT，并由 keyslot 接口销毁。 否则，应该传入由 setup 接口创建的句柄。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_symc_teardown(uint32_t symc_handle, uint32_t keyslot_handle);

/**
 * @}
 */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif  /* SECURITY_SYMC_H */