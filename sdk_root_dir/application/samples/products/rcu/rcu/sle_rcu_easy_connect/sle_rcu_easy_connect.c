/**
 * Copyright (c) Triductor. 2023-2023. All rights reserved.
 *
 * Description: easy connect function for sle rcu server. \n
 *
 * History: \n
 * 2023-09-21, Create file. \n
 */

#include "securec.h"
#include "errcode.h"
#include "osal_addr.h"
#include "osal_task.h"
#include "common_def.h"
#include "sle_common.h"
#include "osal_debug.h"
#include "sle_errcode.h"
#include "osal_timer.h"

#include "sle_device_discovery.h"
#include "sle_connection_manager.h"
#include "sle_ssap_client.h"
#include "sle_ssap_server.h"
#include "sle_rcu_easy_connect.h"

#if defined(CONFIG_SAMPLE_SUPPORT_SLE_RCU_EASY_CONNECT)
#define SLE_EASY_CONNECT_UUID_MAX_NUM 3
#define SLE_EASY_CONNECT_UUID_IDX0 0
#define SLE_EASY_CONNECT_UUID_IDX1 1
#define SLE_EASY_CONNECT_UUID_IDX2 2

static uint8_t g_sle_easy_connect_uuid[SLE_EASY_CONNECT_UUID_MAX_NUM][SLE_UUID_LEN] = {
    /* Close to Discovery Device service UUID. */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0B },
    /* acteristic UUID. 传输设备信息 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x11, 0x3C },
    /* CCCD */
    { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
      0x00, 0x10, 0x00, 0x00, 0x02, 0x29, 0x00, 0x00 },
};

sle_addr_t g_remote_addr = {0};
uint16_t g_service_control_handle = 0;
uint16_t g_control_property_handle = 0;
uint8_t g_sle_rcu_server_id = 0;
uint8_t g_sle_rcu_conn_id = 0;
uint8_t g_device_cnt  = 0;
sle_device_recode g_all_devices[MAX_DEVICE_SIZE] = {0};

void rcu_easy_connect_get_server_id(uint8_t server_id)
{
    g_sle_rcu_server_id = server_id;
}

void rcu_easy_connect_get_conn_id(uint8_t conn_id)
{
    g_sle_rcu_conn_id = conn_id;
}

bool is_device_found(uint8_t *device_mac, int8_t *index)
{
    bool found = false;
    if (device_mac == NULL) {
        printf("device mac is null.\n");
        return found;
    }
    for (int i = 0; i < g_device_cnt; i++) {
        if (memcmp(g_all_devices[i].buf.addr, device_mac, SLE_ADDR_LEN) == 0) {
            found = true;
            *index = i;
            break;
        }
    }
    return found;
}

int update_device_list(int index, sle_seek_result_info_t *scan_result_data)
{
    if (index >= MAX_DEVICE_SIZE) {
        return -1;
    }

    sle_device_recode *cur_dev = &g_all_devices[index];
    uint32_t *scan_pos = &cur_dev->scan_pos;
    if (*scan_pos  >= MAX_RSII_SIZE) {
        *scan_pos = 0;
    }
    cur_dev->type = scan_result_data->addr.type;
    cur_dev->rssi_record[*scan_pos] = scan_result_data->rssi;
    memcpy_s(cur_dev->buf.addr, SLE_ADDR_LEN, scan_result_data->addr.addr, SLE_ADDR_LEN);
    memcpy_s(cur_dev->buf.adv_data, sizeof(cur_dev->buf.adv_data),
        scan_result_data->data, scan_result_data->data_length);
    cur_dev->buf.adv_len = min(scan_result_data->data_length, sizeof(cur_dev->buf.adv_data));

    *scan_pos = (*scan_pos) + 1;
    return index;
}

int push_device_list(sle_seek_result_info_t *scan_result_data)
{
    if (g_device_cnt >= MAX_DEVICE_SIZE) {
        return -1;
    }
    memset_s(&g_all_devices[g_device_cnt], sizeof(sle_device_recode), 0, sizeof(sle_device_recode));
    update_device_list(g_device_cnt, scan_result_data);
    g_device_cnt++;
    return g_device_cnt;
}

bool is_device_near_enough(int8_t rssi, int index)
{
    if (index >= MAX_DEVICE_SIZE) {
        return false;
    }
    unused(rssi);

    sle_device_recode *cur_dev = &g_all_devices[index];
    int is_near_times = 0;

    for (int i = 0; i < MAX_RSII_SIZE; i++) {
        if (cur_dev->rssi_record[i] == 0) {
            continue;
        }
        if (cur_dev->rssi_record[i] >= DEVICE_IS_NEAR_RSSI) {
            is_near_times++;
        }
    }

    if (is_near_times >= DEVICE_IS_NEAR_TIMES) {
        return true;
    } else {
        return false;
    }
}

bool is_connect_time_enough(int index, long cur_time)
{
    long time_gap = cur_time - g_all_devices[index].connect_time;
    if ((time_gap > (TIME_THOUSANDS * CONNECT_TIME_GAP)) || (time_gap < 0)) {
        return true;
    }
    printf("[Demo] Find New Connectable Device, But time is too short.\n");
    return false;
}

void update_device_time(int index, long ms)
{
    g_all_devices[index].connect_time = ms;
}

long get_cur_time(void)
{
    osal_timeval tv;
    osal_gettimeofday(&tv);
    return (tv.tv_sec * TIME_THOUSANDS + tv.tv_usec / TIME_THOUSANDS);
}

void sle_sample_seek_enable_cbk(errcode_t status)
{
    printf("%s enter, status:%d.\n", __FUNCTION__, status);
}

void sle_sample_seek_disable_cbk(errcode_t status)
{
    printf("%s enter, status:%d.\n", __FUNCTION__, status);
}

errcode_t send_device_info(void)
{
    errcode_t ret = ERRCODE_SLE_SUCCESS;
    report_data_info report_data = {0};
    uint8_t vendor_id[]      = {0x12, 0x34};
    uint8_t product_type[]   = {0x01, 0x02};
    uint8_t extra_info[]     = {0x11, 0x22, 0x33, 0x44, 0x55};
    report_data.report_id    = SAMPLE_REPORT_ID;
    report_data.vendor_id    = (uint16_t)((vendor_id[0] << 8) | vendor_id[1]);  /* offset 8bit */
    report_data.product_type = (uint16_t)((product_type[0] << 8) | product_type[1]);  /* offset 8bit */
    memcpy_s(report_data.mac_addr, SLE_ADDR_LEN, g_remote_addr.addr, SLE_ADDR_LEN);
    memcpy_s(report_data.extra_data, MAX_REPORT_DATA_LEN, extra_info, sizeof(extra_info));

    ret = sle_rcu_send_cfg_info_by_handle((uint8_t *)&report_data, sizeof(report_data_info), g_sle_rcu_conn_id);
    if (ret != ERRCODE_SLE_SUCCESS) {
        printf("[Demo] send_device_info fail, ret:%d.\n", ret);
        return ret;
    } else {
        printf("[Demo] send_device_info succ, product_type:%d_%d.\n", product_type[0], product_type[1]);
    }
    return ret;
}


void sle_sample_seek_result_info_cbk(sle_seek_result_info_t *seek_result_data)
{
    int8_t idx = -1;
    int8_t curr_idx = -1;
    errcode_t ret = ERRCODE_SLE_SUCCESS;
    (void)memcpy_s(&g_remote_addr, sizeof(sle_addr_t), &seek_result_data->addr, sizeof(sle_addr_t));
    printf("length:%d, rssi:%d, data:", seek_result_data->data_length, seek_result_data->rssi);
    for (uint8_t i = 0; i < seek_result_data->data_length; i++) {
        printf(" %02x ", seek_result_data->data[i]);
    }
    printf("\n");

    if (is_device_found(seek_result_data->addr.addr, &idx)) {
        update_device_list(idx, seek_result_data);
    } else {
        idx = push_device_list(seek_result_data);
    }

    if ((idx != -1) && is_device_near_enough(seek_result_data->rssi, idx)) {
        long cur_time = get_cur_time();
        if (is_connect_time_enough(idx, cur_time)) {
            update_device_time(idx, cur_time);
            curr_idx = idx;

            ret = sle_stop_seek();  /* sle_sample_seek_disable_cbk */
            if (ret != ERRCODE_SLE_SUCCESS) {
                printf("[%s] sle_stop_seek fail, ret:%x.\n", __FUNCTION__, ret);
                return;
            }

            ret = send_device_info();
            if (ret != ERRCODE_SLE_SUCCESS) {
                printf("[%s] send_device_info fail, ret:%x.\n", __FUNCTION__, ret);
                return;
            } else {
                printf("[%s] send_device_info succ.\n", __FUNCTION__);
                memset_s(&g_all_devices[curr_idx], sizeof(sle_device_recode), 0, sizeof(sle_device_recode));
            }
        }
    }
}

static errcode_t sle_sample_set_uuid(uint8_t *uuid, sle_uuid_t *service_uuid)
{
    if (memcpy_s(service_uuid->uuid, SLE_UUID_LEN, uuid, SLE_UUID_LEN) != EOK) {
        printf("sle mouse hid set uuid fail\r\n");
        return ERRCODE_SLE_MEMCPY_FAIL;
    }
    service_uuid->len = SLE_UUID_LEN;
    return ERRCODE_SLE_SUCCESS;
}

static errcode_t sle_uuid_control_server_service_add(void)
{
    errcode_t ret;
    sle_uuid_t service_uuid = {0};
    ret = sle_sample_set_uuid(g_sle_easy_connect_uuid[SLE_EASY_CONNECT_UUID_IDX0], &service_uuid);
    if (ret != ERRCODE_SLE_SUCCESS) {
        printf("[sle control] sle_uuid_control_server_service_add, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }

    ret = ssaps_add_service_sync(g_sle_rcu_server_id, &service_uuid, 1, &g_service_control_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        printf("[sle control] ssaps_add_service_sync, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }
    printf("[sle control] ssaps_add_service succ, service_handle:%d\r\n", g_service_control_handle);
    return ERRCODE_SLE_SUCCESS;
}

static errcode_t sle_uuid_control_server_property_add(void)
{
    errcode_t ret = ERRCODE_SLE_SUCCESS;
    uint16_t property_handle = 0;
    uint8_t property_value[2] = {0x12, 0x34};
    uint8_t descriptor_value[2] = {0x01, 0x00};

    /* add property interface */
    sle_uuid_t service_uuid = {0};
    ret = sle_sample_set_uuid(g_sle_easy_connect_uuid[SLE_EASY_CONNECT_UUID_IDX1], &service_uuid);
    if (ret != ERRCODE_SLE_SUCCESS) {
        printf("[sle control] sle_uuid_control_server_property_add, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }

    ssaps_property_info_t property = {0};
    property.uuid = service_uuid;
    property.permissions = SSAP_PERMISSION_READ | SSAP_PERMISSION_WRITE;
    property.operate_indication = SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_NOTIFY;
    property.value_len = sizeof(property_value);
    property.value = property_value;
    ret = ssaps_add_property_sync(g_sle_rcu_server_id, g_service_control_handle, &property, &property_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        printf("[sle control] sle ssaps_add_property_sync fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }
    g_control_property_handle = property_handle;

    /* add descriptor interface */
    ret = sle_sample_set_uuid(g_sle_easy_connect_uuid[SLE_EASY_CONNECT_UUID_IDX2], &service_uuid);
    if (ret != ERRCODE_SLE_SUCCESS) {
        printf("[sle control] sle mouse uuid set fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }

    ssaps_desc_info_t descriptor = {0};
    descriptor.uuid = service_uuid;
    descriptor.permissions = SSAP_PERMISSION_READ | SSAP_PERMISSION_WRITE;
    descriptor.operate_indication = SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_NOTIFY;
    descriptor.type = SSAP_DESCRIPTOR_CLIENT_CONFIGURATION;
    descriptor.value_len = sizeof(descriptor_value);
    descriptor.value = descriptor_value;
    ret = ssaps_add_descriptor_sync(g_sle_rcu_server_id, g_service_control_handle, property_handle, &descriptor);
    if (ret != ERRCODE_SLE_SUCCESS) {
        printf("[sle control] sle mouse uuid set fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }

    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_rcu_easy_connect_server_add(void)
{
    errcode_t ret;
    if (sle_uuid_control_server_service_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_sle_rcu_server_id);
        return ERRCODE_SLE_FAIL;
    }
    if (sle_uuid_control_server_property_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_sle_rcu_server_id);
        return ERRCODE_SLE_FAIL;
    }
    printf("%s sle rcu add service, server_id:%x, service_handle:%x, property_handle:%x\r\n",
        __FUNCTION__, g_sle_rcu_server_id, g_service_control_handle, g_control_property_handle);
    ret = ssaps_start_service(g_sle_rcu_server_id, g_service_control_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_rcu_send_cfg_info_by_handle(uint8_t *data, uint8_t len, uint16_t conn_id)
{
    ssaps_ntf_ind_t param = { 0 };
    uint8_t receive_buf[20] = { 0 };  /* max receive length 20. */
    param.handle    = g_control_property_handle;
    param.type      = SSAP_PROPERTY_TYPE_VALUE;
    param.value     = receive_buf;
    param.value_len = len;
    if (memcpy_s(param.value, param.value_len, data, len) != EOK) {
        return ERRCODE_SLE_FAIL;
    }
    return ssaps_notify_indicate(g_sle_rcu_server_id, conn_id, &param);
}

errcode_t sle_start_scan(void)
{
    errcode_t ret;
    sle_seek_param_t param = {0};
    param.own_addr_type = 0;
    param.filter_duplicates = 0;
    param.seek_filter_policy = 0;
    param.seek_phys = 1;
    param.seek_type[0] = 0;
    param.seek_interval[0] = SLE_EASY_CONNECT_SAMPLE_SEEK_INTERVAL;
    param.seek_window[0] = SLE_EASY_CONNECT_SAMPLE_SEEK_WINDOW;
    ret = sle_set_seek_param(&param);
    if (ret != ERRCODE_SLE_SUCCESS) {
        printf("%s sle_set_seek_param fail :%x\r\n", __FUNCTION__, ret);
        return ret;
    }
    ret = sle_start_seek();
    if (ret != ERRCODE_SLE_SUCCESS) {
        printf("%s sle_start_seek fail :%x\r\n", __FUNCTION__, ret);
        return ret;
    }
    return ret;
}

void sle_easy_connect_start(void)
{
    printf("%s sle_easy_connect start scan.\r\n", __FUNCTION__);
    errcode_t ret = 0;
    ret = sle_start_scan();
    if (ret != ERRCODE_SLE_SUCCESS) {
        printf("%s sle_start_scan fail, ret=%x\r\n", __FUNCTION__, ret);
    } else {
        printf("%s sle_start_scan succ\r\n", __FUNCTION__);
    }
}

void sle_easy_connect_stop(void)
{
    printf("%s sle_easy_connect stop scan.\r\n", __FUNCTION__);
    errcode_t ret = 0;
    ret = sle_stop_seek();
    if (ret != ERRCODE_SLE_SUCCESS) {
        printf("%s sle_stop_seek fail, ret=%x\r\n", __FUNCTION__, ret);
    } else {
        printf("%s sle_stop_seek succ\r\n", __FUNCTION__);
    }
}
#else

void rcu_easy_connect_get_server_id(uint8_t server_id)
{
    server_id = server_id;
}

void rcu_easy_connect_get_conn_id(uint8_t conn_id)
{
    conn_id = conn_id;
}

void sle_sample_seek_enable_cbk(errcode_t status)
{
    printf("%s enter, status:%d.\n", __FUNCTION__, status);
}

void sle_sample_seek_disable_cbk(errcode_t status)
{
    printf("%s enter, status:%d.\n", __FUNCTION__, status);
}

void sle_sample_seek_result_info_cbk(sle_seek_result_info_t *seek_result_data)
{
    seek_result_data = seek_result_data;
    return;
}

errcode_t sle_rcu_easy_connect_server_add(void)
{
    return ERRCODE_SLE_SUCCESS;
}

void sle_easy_connect_start(void)
{
    return;
}

void sle_easy_connect_stop(void)
{
    return;
}
#endif  /* CONFIG_SAMPLE_SUPPORT_SLE_RCU_EASY_CONNECT */