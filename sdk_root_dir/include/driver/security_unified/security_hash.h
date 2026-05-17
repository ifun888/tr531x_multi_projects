/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 * Description: Hash simple API header file.
 *
 * Create: 2024-07-02
*/

#ifndef SECURITY_HASH_H
#define SECURITY_HASH_H

#include "errcode.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**
 * @defgroup security_unified_hash Hash
 * @ingroup  drivers_driver_security_unified
 * @{
 */

/* HASH(Single-Part). */
/**
 * @if Eng
 * @brief  SHA1 hash calculate. Single-Part API.
 * @param  [in]   buf Mssage.
 * @param  [in]   len Message length.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  SHA1 摘要计算。 单流程接口。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @param  [out]  out 摘要结果。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha1(const uint8_t *buf, uint32_t len, uint8_t out[20]);

/**
 * @if Eng
 * @brief  SHA224 hash calculate. Single-Part API.
 * @param  [in]   buf Mssage.
 * @param  [in]   len Message length.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  SHA224 摘要计算。 单流程接口。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @param  [out]  out 摘要结果。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha224(const uint8_t *buf, uint32_t len, uint8_t out[28]);

/**
 * @if Eng
 * @brief  SHA256 hash calculate. Single-Part API.
 * @param  [in]   buf Mssage.
 * @param  [in]   len Message length.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  SHA256 摘要计算。 单流程接口。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @param  [out]  out 摘要结果。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha256(const uint8_t *buf, uint32_t len, uint8_t out[32]);

/**
 * @if Eng
 * @brief  SHA384 hash calculate. Single-Part API.
 * @param  [in]   buf Mssage.
 * @param  [in]   len Message length.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  SHA384 摘要计算。 单流程接口。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @param  [out]  out 摘要结果。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha384(const uint8_t *buf, uint32_t len, uint8_t out[48]);

/**
 * @if Eng
 * @brief  SHA512 hash calculate. Single-Part API.
 * @param  [in]   buf Mssage.
 * @param  [in]   len Message length.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  SHA512 摘要计算。 单流程接口。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @param  [out]  out 摘要结果。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha512(const uint8_t *buf, uint32_t len, uint8_t out[64]);

/**
 * @if Eng
 * @brief  SM3 hash calculate. Single-Part API.
 * @param  [in]   buf Mssage.
 * @param  [in]   len Message length.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  SM3 摘要计算。 单流程接口。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @param  [out]  out 摘要结果。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sm3(const uint8_t *buf, uint32_t len, uint8_t out[32]);


/* HASH(Multi-Part) */
/**
 * @if Eng
 * @brief  Create SHA1 hash handle.
 * @param  [out]  hash_handle Hash handle.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  创建 SHA1 hash 句柄。
 * @param  [out]  hash_handle Hash 句柄.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha1_start(uint32_t *hash_handle);

/**
 * @if Eng
 * @brief  Update SHA1 message. Support multi-calling.
 * @param  [in]   hash_handle Hash handle.
 * @param  [in]   buf Message.
 * @param  [in]   len Message length.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  更新 SHA1 消息内容。支持多次调用。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha1_update(uint32_t hash_handle, const uint8_t *buf, uint32_t len);

/**
 * @if Eng
 * @brief  Finish SHA1 hash calculation and get hash result.
 * @param  [in]   hash_handle Hash handle.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  结束 SHA1 hash 计算，并获取摘要结果。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [out]  out 摘要结果.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha1_finish(uint32_t hash_handle, uint8_t out[20]);

/**
 * @if Eng
 * @brief  Create SHA224 hash handle.
 * @param  [out]  hash_handle Hash handle.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  创建 SHA224 hash 句柄。
 * @param  [out]  hash_handle Hash 句柄.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha224_start(uint32_t *hash_handle);

/**
 * @if Eng
 * @brief  Update SHA224 message. Support multi-calling.
 * @param  [in]   hash_handle Hash handle.
 * @param  [in]   buf Message.
 * @param  [in]   len Message length.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  更新 SHA224 消息内容。支持多次调用。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha224_update(uint32_t hash_handle, const uint8_t *buf, uint32_t len);

/**
 * @if Eng
 * @brief  Finish SHA224 hash calculation and get hash result.
 * @param  [in]   hash_handle Hash handle.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  结束 SHA224 hash 计算，并获取摘要结果。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [out]  out 摘要结果.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha224_finish(uint32_t hash_handle, uint8_t out[28]);

/**
 * @if Eng
 * @brief  Create SHA256 hash handle.
 * @param  [out]  hash_handle Hash handle.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  创建 SHA256 hash 句柄。
 * @param  [out]  hash_handle Hash 句柄.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha256_start(uint32_t *hash_handle);

/**
 * @if Eng
 * @brief  Update SHA256 message. Support multi-calling.
 * @param  [in]   hash_handle Hash handle.
 * @param  [in]   buf Message.
 * @param  [in]   len Message length.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  更新 SHA256 消息内容。支持多次调用。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha256_update(uint32_t hash_handle, const uint8_t *buf, uint32_t len);

/**
 * @if Eng
 * @brief  Finish SHA256 hash calculation and get hash result.
 * @param  [in]   hash_handle Hash handle.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  结束 SHA256 hash 计算，并获取摘要结果。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [out]  out 摘要结果.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha256_finish(uint32_t hash_handle, uint8_t out[32]);

/**
 * @if Eng
 * @brief  Create SHA384 hash handle.
 * @param  [out]  hash_handle Hash handle.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  创建 SHA384 hash 句柄。
 * @param  [out]  hash_handle Hash 句柄.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha384_start(uint32_t *hash_handle);

/**
 * @if Eng
 * @brief  Update SHA384 message. Support multi-calling.
 * @param  [in]   hash_handle Hash handle.
 * @param  [in]   buf Message.
 * @param  [in]   len Message length.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  更新 SHA384 消息内容。支持多次调用。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha384_update(uint32_t hash_handle, const uint8_t *buf, uint32_t len);

/**
 * @if Eng
 * @brief  Finish SHA384 hash calculation and get hash result.
 * @param  [in]   hash_handle Hash handle.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  结束 SHA384 hash 计算，并获取摘要结果。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [out]  out 摘要结果.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha384_finish(uint32_t hash_handle, uint8_t out[48]);

/**
 * @if Eng
 * @brief  Create SHA512 hash handle.
 * @param  [out]  hash_handle Hash handle.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  创建 SHA512 hash 句柄。
 * @param  [out]  hash_handle Hash 句柄.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha512_start(uint32_t *hash_handle);

/**
 * @if Eng
 * @brief  Update SHA512 message. Support multi-calling.
 * @param  [in]   hash_handle Hash handle.
 * @param  [in]   buf Message.
 * @param  [in]   len Message length.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  更新 SHA512 消息内容。支持多次调用。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha512_update(uint32_t hash_handle, const uint8_t *buf, uint32_t len);

/**
 * @if Eng
 * @brief  Finish SHA512 hash calculation and get hash result.
 * @param  [in]   hash_handle Hash handle.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  结束 SHA512 hash 计算，并获取摘要结果。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [out]  out 摘要结果.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sha512_finish(uint32_t hash_handle, uint8_t out[64]);

/**
 * @if Eng
 * @brief  Create SM3 hash handle.
 * @param  [out]  hash_handle Hash handle.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  创建 SM3 hash 句柄。
 * @param  [out]  hash_handle Hash 句柄.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sm3_start(uint32_t *hash_handle);

/**
 * @if Eng
 * @brief  Update SM3 message. Support multi-calling.
 * @param  [in]   hash_handle Hash handle.
 * @param  [in]   buf Message.
 * @param  [in]   len Message length.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  更新 SM3 消息内容。支持多次调用。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [in]   buf 消息。
 * @param  [in]   len 消息长度。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sm3_update(uint32_t hash_handle, const uint8_t *buf, uint32_t len);

/**
 * @if Eng
 * @brief  Finish SM3 hash calculation and get hash result.
 * @param  [in]   hash_handle Hash handle.
 * @param  [out]  out Hash result.
 * @retval ERRCODE_SUCC Success.
 * @retval Other        Failure. For details, see @ref errcode_t.
 * @else
 * @brief  结束 SM3 hash 计算，并获取摘要结果。
 * @param  [in]   hash_handle Hash 句柄。
 * @param  [out]  out 摘要结果.
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 * @endif
 */
errcode_t uapi_drv_cipher_sm3_finish(uint32_t hash_handle, uint8_t out[32]);

/**
 * @}
 */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif  /* SECURITY_HASH_H */
