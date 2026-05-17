/*
 * Copyright (c) Triductor. 2021-2023. All rights reserved.
 * Description: transmit header file
 * This file should be changed only infrequently and with great care.
 */
#ifndef TRANSMIT_H
#define TRANSMIT_H

#include <stdint.h>
#include <stdbool.h>
#include "common_def.h"
#include "errcode.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * @defgroup middleware_utils_dfx_transmit Transmit
 * @ingroup  middleware_utils_dfx
 * @{
 */

/**
 * @if Eng
 * @brief  transmit type.
 * @else
 * @brief  传输类型
 * @endif
 */
typedef enum {
    TRANSMIT_TYPE_FILE_UPSTREAM      = 0, /*!< @if Eng File data upstream transmission.
                                               @else  文件数据上行(下位机->上位机)，即下位机作为源端(读文件) @endif */
    TRANSMIT_TYPE_RESERVED           = 1, /*!< @if Eng reserved.
                                               @else  保留类型(为兼容旧的工具而保留) @endif */
    TRANSMIT_TYPE_FILE_DOWNSTREAM    = 2, /*!< @if Eng File data downstream transmission.
                                               @else  文件数据下行(上位机->下位机)，即下位机作为目的端(写文件) @endif */
    TRANSMIT_TYPE_OTA_IMG_DOWNSTREAM = 3, /*!< @if Eng OTA image data downstream transmission.
                                               @else  升级包数据下行(上位机->下位机)，即下位机作为目的端(写升级包) @endif */
    TRANSMIT_TYPE_MEMORY_UPSTREAM    = 4, /*!< @if Eng Memmory data upstream transmission.
                                               @else  内存数据上行(下位机->上位机)，即下位机作为源端(读内存) @endif */
    TRANSMIT_TYPE_MEMORY_DOWNSTREAM  = 5, /*!< @if Eng Memmory data downstream transmission.
                                               @else  内存数据下行(上位机->下位机)，即下位机作为目的端(写内存) @endif */
    TRANSMIT_TYPE_FLASH_UPSTREAM     = 6, /*!< @if Eng Flash data upstream transmission.
                                               @else  Flash数据上行(下位机->上位机)，即下位机作为源端(读Flash) @endif */
    TRANSMIT_TYPE_FLASH_DOWNSTREAM   = 7, /*!< @if Eng Flash data downstream transmission.
                                               @else  Flash数据下行(上位机->下位机)，即下位机作为目的端(写Flash) @endif */
    TRANSMIT_TYPE_OTA_IMG_UPSTREAM   = 8, /*!< @if Eng OTA image data upstream transmission.
                                               @else  升级包数据上行(下位机->上位机)，即下位机作为源端(读升级包) @endif */
    TRANSMIT_TYPE_MAX,                    /*!< @if Eng The number of the transmit types.
                                               @else  传输类型数量 @endif */
} transmit_type_t;

/**
 * @if Eng
 * @brief  Address information of transmitted data
 * @else
 * @brief  传输数据的地址信息
 * @endif
 */
typedef struct {
    uintptr_t host_start_addr;       /*!< @if Eng Start address for flash or memory data on the host end.
                                          @else   上位机上的Flash或memory数据的起始地址 @endif */
    uintptr_t device_start_addr;     /*!< @if Eng Start address for flash or memory data on the device end.
                                          @else   下位机上的Flash或memory数据的起始地址 @endif */
} transmit_addr_info;

/**
 * @if Eng
 * @brief  Name of the file to be transmitted.
 * @else
 * @brief  传输文件的名称
 * @endif
 */
typedef struct {
    const char *host_file_name;    /*!< @if Eng File name (include path) for file data on the host end.
                                        @else   上位机上的文件名称（包含路径） @endif */
    const char *device_file_name;  /*!< @if Eng File name (include path) for file data on the device end.
                                        @else   下位机上文件名称（包含路径） @endif */
} transmit_file_info;

/**
 * @if Eng
 * @brief  Transmission configuration information
 * @else
 * @brief  传输配置信息
 * @endif
 */
typedef struct {
    union {
        transmit_addr_info addr_info;   /*!< @if Eng The address information of Flash or memory data
                                             @else   Flash或memory数据地址信息 @endif */
        transmit_file_info file_info;   /*!< @if Eng The file name info.
                                             @else   文件名称信息 @endif */
    } data;
    uint32_t total_size;                /*!< @if Eng Total size of data to be transmitted.
                                             @else   要传输的数据长度 @endif */
    uint16_t data_block_number;         /*!< @if Eng Number of transmissions per group. The value 0 indicates
                                                     the default value DEFAULT_TRANSMIT_BLOCK_NUMBER.
                                             @else   每组传输次数，0表示默认值DEFAULT_TRANSMIT_BLOCK_NUMBER @endif */
    uint16_t data_block_size;           /*!< @if Eng Size of data transmitted each time. The value 0 indicates
                                                     the default value DEFAULT_TRANSMIT_BLOCK_SIZE.
                                             @else   每次传输数据大小， 0表示默认值DEFAULT_TRANSMIT_BLOCK_SIZE @endif */
    bool re_transmit;                   /*!< @if Eng Whether resumable retransmission is required.
                                             @else   是否需要断点续传 @endif */
} transmit_cfg_info_t;

/**
 * @if Eng
 * @brief  transmission result callback function
 * @else
 * @brief  传输结果回调函数。
 * @endif
 */
typedef errcode_t (*transmit_result_hook)(errcode_t result, uintptr_t usr_data);

/**
 * @if Eng
 * @brief  transmission command processing callback function.
 * @else
 * @brief  传输命令处理回调函数。
 * @endif
 */
typedef errcode_t (*transmit_msg_proc_hook)(uint32_t msg_id, const uint8_t *msg, uint32_t msg_len);

/**
 * @if Eng
 * @brief  Transmission callback information
 * @else
 * @brief  传输回调信息
 * @endif
 */
typedef struct {
    transmit_result_hook result_hook;       /*!< @if Eng user data of transmission result callback function
                                                 @else   传输结果回调函数 @endif */
    uintptr_t result_usr_data;              /*!< @if Eng transmission result callback function
                                                 @else   传输结果回调函数用户数据 @endif */
} transmit_callback_t;


/**
 * @if Eng
 * @brief  Initializing the transmit module.
 * @par Description: Initializing the transmit module.
 * @retval ERRCODE_SUCC            Success.
 * @retval Others                  ERRCODE_FAIL or other error num.
 * @else
 * @brief  初始化数据传输模块。
 * @par 说明: 初始化数据传输模块。
 * @retval ERRCODE_SUCC           成功返回#ERRCODE_SUCC。
 * @retval Others                 失败返回#ERRCODE_FAIL或其他返回值。
 * @endif
 */
errcode_t uapi_transmit_init(void);

/**
 * @if Eng
 * @brief  Deinitialize the transmit module.
 * @par Description: Deinitialize the transmit module.
 * @retval ERRCODE_SUCC            Success.
 * @retval Others                  ERRCODE_FAIL or other error num.
 * @else
 * @brief  去初始化数据传输模块。
 * @par 说明: 去初始化数据传输模块。
 * @retval ERRCODE_SUCC           成功返回#ERRCODE_SUCC。
 * @retval Others                 失败返回#ERRCODE_FAIL或其他返回值。
 * @endif
 */
errcode_t uapi_transmit_deinit(void);

/**
 * @if Eng
 * @brief  Start a transmission as a host end.
 * @par Description: In a complete transmission, transmission is started by the host end.
 *                   regardless of upstream or downstream.
 *                   CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE needs to be YES to use this function.
 * @param  [in]  transmit_type transmission type. see @ref transmit_type_t
 * @param  [in]  channel_id Channel ID for transmission. see diag_frame_fid_t
 * @param  [in]  cfg_info info of the data(such as file name, address, and size). see @ref transmit_cfg_info_t
 * @param  [in]  callback Callback functions of the transmission. see @ref transmit_callback_t
 * @retval ERRCODE_SUCC            Success.
 * @retval Others                  ERRCODE_FAIL or other error num.
 * @else
 * @brief  作为上位机启动一次传输。
 * @par 说明: 在一次完整传输中，数据不论上行还是下行，均由上位机启动传输。使用此函数需打开CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE宏。
 * @param  [in]  transmit_type 传输类型。 参考 @ref transmit_type_t
 * @param  [in]  channel_id 传输所使用的通道ID。参考 diag_frame_fid_t
 * @param  [in]  cfg_info 要传输的数据的配置信息(如文件名、地址、长度等)。参考 @ref transmit_cfg_info_t
 * @param  [in]  callback 传输回调函数。参考 @ref transmit_callback_t
 * @retval ERRCODE_SUCC           成功返回#ERRCODE_SUCC。
 * @retval Others                 失败返回#ERRCODE_FAIL或其他返回值。
 * @endif
 */
errcode_t uapi_transmit_host_start(transmit_type_t transmit_type, uint16_t channel_id, transmit_cfg_info_t *cfg_info,
    transmit_callback_t *callback);

/**
 * @if Eng
 * @brief  Stop transmission as host end.
 * @par Description: The host end can actively stop the transmission in the transmission process.
 *                   CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE needs to be YES to use this function.
 * @param  [in]  transmit_type transmission type. see @ref transmit_type_t
 * @param  [in]  channel_id Channel ID for transmission. see diag_frame_fid_t
 * @retval ERRCODE_SUCC            Success.
 * @retval Others                  ERRCODE_FAIL or other error num.
 * @else
 * @brief  作为上位机主动停止传输。
 * @par 说明: 上位机可以在传输过程中主动停止传输。使用此函数需打开CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE宏。
 * @param  [in]  transmit_type 传输类型。 参考 @ref transmit_type_t
 * @param  [in]  channel_id 传输所使用的通道ID。参考 diag_frame_fid_t
 * @retval ERRCODE_SUCC           成功返回#ERRCODE_SUCC。
 * @retval Others                 失败返回#ERRCODE_FAIL或其他返回值。
 * @endif
 */
errcode_t uapi_transmit_host_stop(transmit_type_t transmit_type, uint16_t channel_id);

/**
 * @if Eng
 * @brief  Stop all transmissions as device end.
 * @par Description: In the normal transmissions process, the transmissions stop is controlled by the host end.
 *                   However, in some special cases (such as connection interruption), the device end can actively
 *                   stop the transmissions by using this function.
 * @retval ERRCODE_SUCC            Success.
 * @retval Others                  ERRCODE_FAIL or other error num.
 * @else
 * @brief  作为下位机停止所有传输。
 * @par 说明: 在正常传输流程中，传输停止是由上位机控制的，但在某些特殊情况下（如连接中断），下位机可以通过此函数
 *            主动停止下位机上的所有传输。
 * @retval ERRCODE_SUCC           成功返回#ERRCODE_SUCC。
 * @retval Others                 失败返回#ERRCODE_FAIL或其他返回值。
 * @endif
 */
errcode_t uapi_transmit_device_stop(void);

/**
 * @if Eng
 * @brief  Registers the transmission callback functions as device end.
 * @par Description:
 * @param  [in]  callback callback functions of the transmission. see @ref transmit_callback_t
 * @retval ERRCODE_SUCC            Success.
 * @retval Others                  ERRCODE_FAIL or other error num.
 * @else
 * @brief  作为下位机注册传输回调函数
 * @par 说明:
 * @param  [in]  callback 传输回调函数。参考 @ref transmit_callback_t
 * @retval ERRCODE_SUCC           成功返回#ERRCODE_SUCC。
 * @retval Others                 失败返回#ERRCODE_FAIL或其他返回值。
 * @endif
 */
errcode_t uapi_transmit_device_register_result_hook(transmit_callback_t *callback);

/**
 * @if Eng
 * @brief  Register a user-defined function for processing transmission messages.
 * @par Description: This function is used to add user-defined messages and process function to the transmit thread.
 *                   CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK needs to be YES to use this function. In addition,
 *                   the transmit thread must be created independently and cannot use the dfx_msg thread.
 * @param  [in]  msg_id_start start ID of the user-defined message.
 * @param  [in]  msg_id_end end ID of the user-defined message.
 * @param  [in]  hook message processing function for user-defined message.
 * @retval ERRCODE_SUCC            Success.
 * @retval Others                  ERRCODE_FAIL or other error num.
 * @else
 * @brief  注册用户自定义的传输消息处理函数。
 * @par 说明: 此函数用于用户在传输线程中添加一些自定义消息及处理函数。使用此函数需打开CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK宏，
 *            且必须单独创建传输线程，不能复用dfx_msg线程。
 * @param  [in]  msg_id_start 自定义消息的起始ID。
 * @param  [in]  msg_id_end 自定义消息的结束ID。
 * @param  [in]  hook 自定义消息处理函数。
 * @retval ERRCODE_SUCC           成功返回#ERRCODE_SUCC。
 * @retval Others                 失败返回#ERRCODE_FAIL或其他返回值。
 * @endif
 */
errcode_t uapi_transmit_register_msg_proc_hook(uint32_t msg_id_start, uint32_t msg_id_end, transmit_msg_proc_hook hook);

/**
 * @if Eng
 * @brief  Unregister user-defined processing function.
 * @par Description: This function is used to unregister user-defined processing functions.
 *                   CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK needs to be YES to use this function.
 * @param  [in]  hook message processing function for user-defined message.
 * @retval ERRCODE_SUCC            Success.
 * @retval Others                  ERRCODE_FAIL or other error num.
 * @else
 * @brief  去注册用户自定义的传输消息处理函数。
 * @par 说明: 此函数用于去注册用户添加的自定义处理函数。使用此函数需打开CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK宏。
 * @param  [in]  hook 自定义消息处理函数。
 * @retval ERRCODE_SUCC           成功返回#ERRCODE_SUCC。
 * @retval Others                 失败返回#ERRCODE_FAIL或其他返回值。
 * @endif
 */
errcode_t uapi_transmit_unregister_msg_proc_hook(transmit_msg_proc_hook hook);

/**
 * @}
 */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* TRANSMIT_H */
