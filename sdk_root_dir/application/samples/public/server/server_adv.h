/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: SLE ADV Config. \n
 * Author: Triductor \n
 * History: \n
 * 2023-07-17, Create file. \n
 */

#ifndef SERVER_ADV_H
#define SERVER_ADV_H
#include "msg.h"

#define SLE_SERVER_ADV_NAME_MAX                 32
#define SLE_SERVER_ADV_VALUE_SIZE                   32
#define SLE_ADV_DATA_TYPE_APPEARANCE              0x07
#define SLE_ADV_DATA_TYPE_COMPLETE_LOCAL_NAME_V11 0x06

typedef struct sle_adv_common_value {
    uint8_t type;
    uint8_t length;
    uint8_t value[SLE_SERVER_ADV_VALUE_SIZE];
} le_adv_common_t;

typedef enum sle_adv_channel {
    SLE_ADV_CHANNEL_MAP_77                 = 0x01,
    SLE_ADV_CHANNEL_MAP_78                 = 0x02,
    SLE_ADV_CHANNEL_MAP_79                 = 0x04,
    SLE_ADV_CHANNEL_MAP_DEFAULT            = 0x07
} sle_adv_channel_map_t;

typedef enum sle_adv_data {
    SLE_ADV_DATA_TYPE_DISCOVERY_LEVEL                              = 0x01,   /* 发现等级 */
    SLE_ADV_DATA_TYPE_ACCESS_MODE                                  = 0x02,   /* 接入层能力 */
    SLE_ADV_DATA_TYPE_SERVICE_DATA_16BIT_UUID                      = 0x03,   /* 标准服务数据信息 */
    SLE_ADV_DATA_TYPE_SERVICE_DATA_128BIT_UUID                     = 0x04,   /* 自定义服务数据信息 */
    SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_16BIT_SERVICE_UUIDS         = 0x05,   /* 完整标准服务标识列表 */
    SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_128BIT_SERVICE_UUIDS        = 0x06,   /* 完整自定义服务标识列表 */
    SLE_ADV_DATA_TYPE_INCOMPLETE_LIST_OF_16BIT_SERVICE_UUIDS       = 0x07,   /* 部分标准服务标识列表 */
    SLE_ADV_DATA_TYPE_INCOMPLETE_LIST_OF_128BIT_SERVICE_UUIDS      = 0x08,   /* 部分自定义服务标识列表 */
    SLE_ADV_DATA_TYPE_SERVICE_STRUCTURE_HASH_VALUE                 = 0x09,   /* 服务结构散列值 */
    SLE_ADV_DATA_TYPE_SHORTENED_LOCAL_NAME                         = 0x0A,   /* 设备缩写本地名称 */
    SLE_ADV_DATA_TYPE_COMPLETE_LOCAL_NAME                          = 0x0B,   /* 设备完整本地名称 */
    SLE_ADV_DATA_TYPE_TX_POWER_LEVEL                               = 0x0C,   /* 广播发送功率 */
    SLE_ADV_DATA_TYPE_SLB_COMMUNICATION_DOMAIN                     = 0x0D,   /* SLB通信域域名 */
    SLE_ADV_DATA_TYPE_SLB_MEDIA_ACCESS_LAYER_ID                    = 0x0E,   /* SLB媒体接入层标识 */
    SLE_ADV_DATA_TYPE_EXTENDED                                     = 0xFE,   /* 数据类型扩展 */
    SLE_ADV_DATA_TYPE_MANUFACTURER_SPECIFIC_DATA                   = 0xFF    /* 厂商自定义信息 */
} sle_adv_data_type;

errcode_t sle_server_adv_init(void);
void sle_server_announce_enable_cbk(uint32_t announce_id, errcode_t status);
void sle_server_announce_disable_cbk(uint32_t announce_id, errcode_t status);
void sle_server_announce_terminal_cbk(uint32_t announce_id);
void sle_config_announce_adv_param(announce_param_t *announce_adv_param);
void sle_config_server_own_addr(sle_addr_t *local_addr);
void sle_config_server_adv_name(uint8_t *adv_name);
int sle_set_default_announce_param(void);
int sle_set_default_announce_data(void);
#endif
