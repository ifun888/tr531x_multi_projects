/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: sle adv config for sle server. \n
 * Author: Triductor \n
 * History: \n
 * 2023-07-17, Create file. \n
 */
#include "securec.h"
#include "errcode.h"
#include "osal_addr.h"
#include "sle_common.h"
#include "sle_device_discovery.h"
#include "sle_errcode.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "string.h"
#include "../msg/msg.h"
#include "key_id.h"
#include "common.h"
#include "nv.h"
#include "server_adv.h"

/* sle device name */
#define NAME_MAX_LENGTH 64
/* 连接调度间隔12.5ms，单位125us */
#define SLE_CONN_INTV_MIN_DEFAULT                 0x64
/* 连接调度间隔12.5ms，单位125us */
#define SLE_CONN_INTV_MAX_DEFAULT                 0x64
/* 连接调度间隔25ms，单位125us */
#define SLE_ADV_INTERVAL_MIN_DEFAULT              0xC8
/* 连接调度间隔25ms，单位125us */
#define SLE_ADV_INTERVAL_MAX_DEFAULT              0xC8
/* 超时时间5000ms，单位10ms */
#define SLE_CONN_SUPERVISION_TIMEOUT_DEFAULT      0x1F4
/* 超时时间4990ms，单位10ms */
#define SLE_CONN_MAX_LATENCY                      0x1F3
/* 广播发送功率 */
#define SLE_ADV_TX_POWER  10
/* 广播ID */
#define SLE_ADV_HANDLE_DEFAULT                    1
/* 最大广播数据长度 */
#define SLE_ADV_DATA_LEN_MAX                      251

#define SLE_LOCAL_SERVER_ADDR                     0xAA
#define SLE_SERVER_LOG "[sle server]"

static uint32_t g_announce_interval_min = SLE_ADV_INTERVAL_MIN_DEFAULT;
static uint32_t g_announce_interval_max = SLE_ADV_INTERVAL_MAX_DEFAULT;
static uint16_t g_conn_interval_min = SLE_CONN_INTV_MIN_DEFAULT;
static uint16_t g_conn_interval_max = SLE_CONN_INTV_MAX_DEFAULT;
static uint8_t g_local_addr[SLE_ADDR_LEN] = { 0x00, 0x6C, 0x01, 0x00, 0x00, 0x01};
static uint8_t g_local_name[SLE_SERVER_ADV_NAME_MAX] = { "SLE_TRIDCUTOR_SERVER" };

static uint16_t sle_set_adv_local_name(uint8_t *adv_data, uint16_t max_len)
{
    errno_t ret;
    uint8_t index = 0;
    uint8_t sle_name_default[SLE_SERVER_ADV_NAME_MAX] = { "TriductorSLE"};
    uint16_t key = SLE_SAMPLE_NV_ID;
    uint16_t key_len = (uint16_t)sizeof(sle_sample_data_config_stru_t);
    uint16_t real_len = 0;
    uint8_t *read_value = osal_vmalloc(key_len);
    if (uapi_nv_read(key, key_len, &real_len, read_value) != ERRCODE_SUCC) {
        /* ERROR PROCESS */
        osal_vfree(read_value);
        read_value = NULL;
        return 0;
    }
    sle_sample_data_config_stru_t *sle_sample_data_t = (sle_sample_data_config_stru_t *)read_value;
    if(memcmp(sle_sample_data_t->sle_name, sle_name_default, strlen(( char *)sle_name_default)) != 0) {
        (void)memcpy_s(g_local_name, strlen(( char *)sle_sample_data_t->sle_name) + 1, sle_sample_data_t->sle_name,
            strlen(( char *)sle_sample_data_t->sle_name)  + 1);
    }
    if (read_value != NULL) {
        osal_vfree(read_value);
        read_value = NULL;
    }
    uint8_t local_name_len = (uint8_t)strlen((char *)g_local_name);
    osal_printk("%s local_name:%s\r\n", SLE_SERVER_LOG, g_local_name);
    adv_data[index++] = SLE_ADV_DATA_TYPE_COMPLETE_LOCAL_NAME_V11;
    adv_data[index++] = local_name_len;
    ret = memcpy_s(&adv_data[index], max_len - index, g_local_name, local_name_len);
    if (ret != EOK) {
        osal_printk("%s memcpy fail\r\n", SLE_SERVER_LOG);
        return 0;
    }
    return (uint16_t)index + local_name_len;
}

static uint16_t sle_set_adv_data(uint8_t *adv_data)
{
    size_t len = 0;
    uint16_t idx = 0;
    errno_t  ret = 0;
    uint8_t adv_dis_service_data_idx = 0;
    uint8_t adv_local_name_len = 0;
    struct sle_adv_common_value adv_disc_level = {
        .type = SLE_ADV_DATA_TYPE_DISCOVERY_LEVEL,
        .length = 0x01,
        .value = {SLE_ANNOUNCE_LEVEL_NORMAL},
    };
    len = adv_disc_level.length + 2;
    ret |= memcpy_s(&adv_data[idx], SLE_ADV_DATA_LEN_MAX - idx, &adv_disc_level, len);
    idx += len;

    struct sle_adv_common_value adv_hid_service = {
        .type = SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_16BIT_SERVICE_UUIDS,
        .length = 0x02,
        .value = {0x0B, 0x06},
    };
    len = adv_hid_service.length + 2;
    ret |= memcpy_s(&adv_data[idx], SLE_ADV_DATA_LEN_MAX - idx, &adv_hid_service, len);
    idx += len;

    struct sle_adv_common_value adv_dis_service_data = {
        .type = SLE_ADV_DATA_TYPE_SERVICE_DATA_16BIT_UUID,
        .length = 0x02,
        .value = {0x09, 0x06},
    };
    len = adv_dis_service_data.length + 2;
    ret |= memcpy_s(&adv_data[idx], SLE_ADV_DATA_LEN_MAX - idx, &adv_dis_service_data, len);
    /* save the adv dis length idx */
    adv_dis_service_data_idx = idx + 1;
    idx += len;

    /* set local name */
    adv_local_name_len = sle_set_adv_local_name(&adv_data[idx], SLE_ADV_DATA_LEN_MAX - idx);
    idx += adv_local_name_len;

    struct sle_adv_common_value adv_appearance_data = {
        .type = SLE_ADV_DATA_TYPE_APPEARANCE,
        .length = 0x03,
        .value = {0x02, 0x05, 0x00},   /*mouse*/
    };
    len = adv_appearance_data.length + 2;
    ret |= memcpy_s(&adv_data[idx], SLE_ADV_DATA_LEN_MAX - idx, &adv_appearance_data, len);
    idx += len;

    adv_data[adv_dis_service_data_idx] = adv_dis_service_data.length + adv_local_name_len + adv_appearance_data.length + 2;
    return idx;
}

static uint16_t sle_set_scan_response_data(uint8_t *scan_rsp_data)
{
    uint16_t idx = 0;
    errno_t ret;
    size_t scan_rsp_data_len = 0;

    struct sle_adv_common_value tx_power_level = {
        .type = SLE_ADV_DATA_TYPE_TX_POWER_LEVEL,
        .length = 0x01,
        .value = {SLE_ADV_TX_POWER}
    };
    scan_rsp_data_len = tx_power_level.length + 2;
    ret = memcpy_s(scan_rsp_data, SLE_ADV_DATA_LEN_MAX, &tx_power_level, scan_rsp_data_len);
    if (ret != EOK) {
        osal_printk("%s sle scan response data memcpy fail\r\n", SLE_SERVER_LOG);
        return 0;
    }
    idx += scan_rsp_data_len;

    /* set local name */
    idx += sle_set_adv_local_name(&scan_rsp_data[idx], SLE_ADV_DATA_LEN_MAX - idx);
    return idx;
}

int sle_set_default_announce_param(void)
{
    sle_announce_param_t param = {0};
    uint8_t index;
    uint8_t sle_addr_default[SLE_ADDR_SIZE] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint16_t key = SLE_SAMPLE_NV_ID;
    uint16_t key_len = (uint16_t)sizeof(sle_sample_data_config_stru_t);
    uint16_t real_len = 0;
    uint8_t *read_value = osal_vmalloc(key_len);
    
    param.announce_mode = SLE_ANNOUNCE_MODE_CONNECTABLE_SCANABLE;
    param.announce_handle = SLE_ADV_HANDLE_DEFAULT;
    param.announce_gt_role = SLE_ANNOUNCE_ROLE_T_CAN_NEGO;
    param.announce_level = SLE_ANNOUNCE_LEVEL_NORMAL;
    param.announce_channel_map = SLE_ADV_CHANNEL_MAP_DEFAULT;
    param.announce_interval_min = g_announce_interval_min;
    param.announce_interval_max = g_announce_interval_max;
    param.conn_interval_min = g_conn_interval_min;
    param.conn_interval_max = g_conn_interval_max;
    param.conn_max_latency = SLE_CONN_MAX_LATENCY;
    param.conn_supervision_timeout = SLE_CONN_SUPERVISION_TIMEOUT_DEFAULT;
    param.own_addr.type = 0;

    if (uapi_nv_read(key, key_len, &real_len, read_value) != ERRCODE_SUCC) {
        /* ERROR PROCESS */
        osal_vfree(read_value);
        read_value = NULL;
        return 0;
    }
    sle_sample_data_config_stru_t *sle_sample_data_t = (sle_sample_data_config_stru_t *)read_value;
    if(memcmp(sle_sample_data_t->sle_own_addr, sle_addr_default, SLE_ADDR_SIZE) == 0){
        (void)memcpy_s(param.own_addr.addr, SLE_ADDR_SIZE, g_local_addr, SLE_ADDR_SIZE);
    }else{
        (void)memcpy_s(param.own_addr.addr, SLE_ADDR_SIZE, sle_sample_data_t->sle_own_addr, SLE_ADDR_SIZE);
    }
    if (read_value != NULL) {
        osal_vfree(read_value);
        read_value = NULL;
    }
    
    osal_printk("%s sle_local addr: ", SLE_SERVER_LOG);
    for (index = 0; index < SLE_ADDR_LEN; index++) {
        osal_printk("0x%02x ", param.own_addr.addr[index]);
    }
    osal_printk("\r\n");
    return sle_set_announce_param(param.announce_handle, &param);
}

int sle_set_default_announce_data(void)
{
    errcode_t ret;
    uint8_t announce_data_len = 0;
    uint8_t seek_data_len = 0;
    sle_announce_data_t data = {0};
    uint8_t adv_handle = SLE_ADV_HANDLE_DEFAULT;
    uint8_t announce_data[SLE_ADV_DATA_LEN_MAX] = {0};
    uint8_t seek_rsp_data[SLE_ADV_DATA_LEN_MAX] = {0};

    announce_data_len = sle_set_adv_data(announce_data);
    data.announce_data = announce_data;
    data.announce_data_len = announce_data_len;

    seek_data_len = sle_set_scan_response_data(seek_rsp_data);
    data.seek_rsp_data = seek_rsp_data;
    data.seek_rsp_data_len = seek_data_len;

    ret = sle_set_announce_data(adv_handle, &data);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s set adv param fail.\r\n", SLE_SERVER_LOG);
    }
    return ERRCODE_SLE_SUCCESS;
}

void sle_server_announce_enable_cbk(uint32_t announce_id, errcode_t status)
{
    osal_printk("%s announce enable id:%d, state:%x\r\n", SLE_SERVER_LOG, announce_id, status);
}

void sle_server_announce_disable_cbk(uint32_t announce_id, errcode_t status)
{
    osal_printk("%s announce disable id:%d, state:%x\r\n", SLE_SERVER_LOG, announce_id, status);
    app_message_t msg_node = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_STOP_ANNOUNCE_SUCCESS;
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

void sle_server_announce_terminal_cbk(uint32_t announce_id)
{
    osal_printk("%s announce terminal id:%d\r\n", SLE_SERVER_LOG, announce_id);
}

void sle_config_announce_adv_param(announce_param_t *announce_adv_param)
{
    if(announce_adv_param == NULL) {
        osal_printk("%s sle_config_announce_adv_param is NULL!\r\n", SLE_SERVER_LOG);
        return;
    }
    g_announce_interval_min = announce_adv_param->announce_interval_min;
    g_announce_interval_max = announce_adv_param->announce_interval_max;
    g_conn_interval_min = announce_adv_param->conn_interval_min;
    g_conn_interval_max = announce_adv_param->conn_interval_max;
}

void sle_config_server_own_addr(sle_addr_t *local_addr)
{
    if(local_addr == NULL) {
        osal_printk("%s sle_config_server_own_addr is NULL!\r\n", SLE_SERVER_LOG);
        return;
    }
    (void)memcpy_s(g_local_addr, SLE_ADDR_LEN, local_addr->addr, SLE_ADDR_LEN);
}

void sle_config_server_adv_name(uint8_t *adv_name)
{
    if(adv_name == NULL) {
        osal_printk("%s sle_config_server_adv_name is NULL!\r\n", SLE_SERVER_LOG);
        return;
    }
    (void)memcpy_s(g_local_name, SLE_SERVER_ADV_NAME_MAX, adv_name, strlen((char *)adv_name) + 1);
}

errcode_t sle_server_adv_init(void)
{
    errcode_t ret;
    sle_set_default_announce_param();
    sle_set_default_announce_data();
    ret = sle_start_announce(SLE_ADV_HANDLE_DEFAULT);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_server_adv_init,sle_start_announce fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}
