/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: SLE Server SOURCE \n
 *
 * History: \n
 * 2024-5-25, Create file. \n
 */
#include "securec.h"
#include "common_def.h"
#include "sle_errcode.h"
#include "sle_connection_manager.h"
#include "sle_device_manager.h"
#include "sle_device_discovery.h"
#include "rcu.h"
#include "sle_rcu_server_adv.h"
#include "sle_ota.h"
#include "osal_addr.h"
#include "sle_vdt_codec.h"
#include "sle_service_hids.h"
#include "sle_service_dis.h"
#include "sle_service_bas.h"
#include "adc_porting.h"
#include "sle_rcu_easy_connect.h"
#include "app_status.h"
#include "sle_server.h"

#define UUID_LEN_2                      2
#define UUID_INDEX                      14
#define OCTET_BIT_LEN                   8
#define BT_INDEX_4                      4
#define BT_INDEX_0                      0
#define SLE_ADV_HANDLE_DEFAULT          1
#define RCU_SEND_BUFF_LENGTH       20
#define HID_ELEMENT_NUM            6
#define SLE_HID_POINT 1

/* sle pair acb handle */
static uint16_t g_sle_pair_handle;
bool g_ssaps_ready = false;
static int g_conn_update = 0;
/* sle server app uuid */
static uint8_t g_sle_uuid_app_uuid[UUID_LEN_2] = { 0x12, 0x34 };
/* server notify property uuid */
static uint8_t g_sle_property_value[OCTET_BIT_LEN] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
/* sle connect acb handle */
static uint16_t g_sle_conn_handle[CONFIG_SLE_MULTICON_NUM] = { 0 };
static uint16_t g_sle_conn_num = 0;
/* sle server handle */
static uint8_t g_server_id = 0;
/* sle service handle */
static uint16_t g_service_handle = 0;
static uint16_t g_service_amic_handle = 0;
/* sle ntf property handle */
static uint16_t g_property_handle = 0;
static uint16_t g_amic_property_handle = 0;
static uint16_t g_sle_conn_id;
static sle_addr_t g_sle_addr = { 0 };
/* 低功耗连接参数信息 */
static sle_connection_param_update_t g_work_to_standby = { 0, 100, 100, 48, 3000 };
static sle_connection_param_update_t g_standby_to_work = { 0, 100, 100, 3, 3000 };
static bool g_low_power_state = false; /* false:关闭, true:打开 */

static uint8_t g_cccd[2] = {0x01, 0x0};
static sle_item_handle_t g_service_hdl[SLE_HID_INDEX_MAX] = {0};

#define input(size)             (0x80 | (size))
#define output(size)            (0x90 | (size))
#define feature(size)           (0xb0 | (size))
#define collection(size)        (0xa0 | (size))
#define end_collection(size)    (0xc0 | (size))

/* Global items */
#define usage_page(size)        (0x04 | (size))
#define logical_minimum(size)   (0x14 | (size))
#define logical_maximum(size)   (0x24 | (size))
#define physical_minimum(size)  (0x34 | (size))
#define physical_maximum(size)  (0x44 | (size))
#define uint_exponent(size)     (0x54 | (size))
#define uint(size)              (0x64 | (size))
#define report_size(size)       (0x74 | (size))
#define report_id(size)         (0x84 | (size))
#define report_count(size)      (0x94 | (size))
#define push(size)              (0xa4 | (size))
#define pop(size)               (0xb4 | (size))

/* Local items */
#define usage(size)                 (0x08 | (size))
#define usage_minimum(size)         (0x18 | (size))
#define usage_maximum(size)         (0x28 | (size))
#define designator_index(size)      (0x38 | (size))
#define designator_minimum(size)    (0x48 | (size))
#define designator_maximum(size)    (0x58 | (size))
#define string_index(size)          (0x78 | (size))
#define string_minimum(size)        (0x88 | (size))
#define string_maximum(size)        (0x98 | (size))
#define delimiter(size)             (0xa8 | (size))

static uint8_t g_sle_report_map_datas[] = {
    usage_page(1),      0x01,
    usage(1),           0x06,
    collection(1),      0x01,
    report_id(1),       0x01,
    usage_page(1),      0x07,
    usage_minimum(1),   0xE0,
    usage_maximum(1),   0xE7,
    logical_minimum(1), 0x00,
    logical_maximum(1), 0x01,
    report_size(1),     0x01,
    report_count(1),    0x08,
    input(1),           0x02,
    report_count(1),    0x01,
    report_size(1),     0x08,
    input(1),           0x01,
    report_count(1),    0x05,
    report_size(1),     0x01,
    usage_page(1),      0x08,
    usage_minimum(1),   0x01,
    usage_maximum(1),   0x05,
    output(1),          0x02,
    report_count(1),    0x01,
    report_size(1),     0x03,
    output(1),          0x01,
    report_count(1),    0x06,
    report_size(1),     0x08,
    logical_minimum(1), 0x00,
    logical_maximum(1), 0xFF,
    usage_page(1),      0x07,
    usage_minimum(1),   0x00,
    usage_maximum(1),   0xFF,
    input(1),           0x00,
    end_collection(0),

    usage_page(1),      0x0C,
    usage(1),           0x01,
    collection(1),      0x01,
    report_id(1),       0x03,
    logical_minimum(1), 0x00,
    logical_maximum(2), 0xff, 0x1f,
    usage_minimum(1),   0x00,
    usage_maximum(2),   0xff, 0x1f,
    report_size(1),     0x10,
    report_count(1),    0x01,
    input(1),           0x00,
    end_collection(0),

    usage_page(1),      0x01,
    usage(1),           0x09,
    collection(1),      0x01,
    report_id(1),       0x02,
    usage(1),           0x81,
    logical_minimum(1), 0x00,
    logical_maximum(2), 0xff, 0x1f,
    usage_minimum(1),   0x00,
    usage_maximum(2),   0xff, 0x1f,
    report_size(1),     0x10,
    report_count(1),    0x01,
    input(1),           0x00,
    end_collection(0),

    usage_page(1),      0x01,
    usage(1),           0x02,
    collection(1),      0x01,
    report_id(1),       0x04,
    usage(1),           0x01,
    collection(1),      0x00,
    report_count(1),    0x03,
    report_size(1),     0x01,
    usage_page(1),      0x09,
    usage_minimum(1),   0x1,
    usage_maximum(1),   0x3,
    logical_minimum(1), 0x00,
    logical_maximum(1), 0x01,
    input(1),           0x02,
    report_count(1),    0x01,
    report_size(1),     0x05,
    input(1),           0x01,
    report_count(1),    0x03,
    report_size(1),     0x08,
    usage_page(1),      0x01,
    usage(1),           0x30,
    usage(1),           0x31,
    usage(1),           0x38,
    logical_minimum(1), 0x81,
    logical_maximum(1), 0x7f,
    input(1),           0x06,
    end_collection(0),
    end_collection(0),
};

uint8_t g_out_low_latency_data[LOW_LATENCY_DATA_MAX] = { 0 };
/* sle gamepad conn state */

static uint8_t g_sle_rcu_base[] = { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, \
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static void encode2byte_little(uint8_t *_ptr, uint16_t data)
{
    *(uint8_t *)((_ptr) + 1) = (uint8_t)((data) >> 0x8);
    *(uint8_t *)(_ptr) = (uint8_t)(data);
}

bool get_g_ssaps_ready(void)
{
    return g_ssaps_ready;
}

uint16_t rcu_get_handle(void)
{
    return g_property_handle;
}

uint16_t get_g_connid(void)
{
    return g_sle_conn_id;
}

uint8_t rcu_get_server_id(void)
{
    return g_server_id;
}

int get_g_conn_update(void)
{
    return g_conn_update;
}

uint16_t get_g_sle_conn_hdl(uint32_t index)
{
    return g_sle_conn_handle[index];
}

uint16_t get_g_sle_conn_num(void)
{
    return g_sle_conn_num;
}

static void sle_uuid_set_base(sle_uuid_t *out)
{
    if (memcpy_s(out->uuid, SLE_UUID_LEN, g_sle_rcu_base, SLE_UUID_LEN) != EOK) {
        out->len = 0;
        return ;
    }
    out->len = UUID_LEN_2;
}

static void sle_uuid_setu2(uint16_t u2, sle_uuid_t *out)
{
    sle_uuid_set_base(out);
    out->len = UUID_LEN_2;
    encode2byte_little(&out->uuid[UUID_INDEX], u2);
}

static void ssaps_mtu_changed_cbk(uint8_t server_id, uint16_t conn_id,  ssap_exchange_info_t *mtu_size,
                                  errcode_t status)
{
    osal_printk("%s ssaps ssaps_mtu_changed_cbk callback server_id:%x, conn_id:%x, mtu_size:%x, status:%x\r\n",
                SLE_RCU_SERVER_LOG, server_id, conn_id, mtu_size->mtu_size, status);
    g_ssaps_ready = true;
    if (g_sle_pair_handle == 0) {
        g_sle_pair_handle =  conn_id + 1;
    }
}

static void ssaps_start_service_cbk(uint8_t server_id, uint16_t handle, errcode_t status)
{
    osal_printk("%s start service cbk callback server_id:%d, handle:%x, status:%x\r\n", SLE_RCU_SERVER_LOG,
                server_id, handle, status);
}

static void ssaps_add_service_cbk(uint8_t server_id, sle_uuid_t *uuid, uint16_t handle, errcode_t status)
{
    unused(uuid);
    osal_printk("%s add service cbk callback server_id:%x, handle:%x, status:%x\r\n", SLE_RCU_SERVER_LOG,
                server_id, handle, status);
}

static void ssaps_add_property_cbk(uint8_t server_id, sle_uuid_t *uuid, uint16_t service_handle,
    uint16_t handle, errcode_t status)
{
    unused(uuid);
    osal_printk("%s add property cbk callback server_id:%x, service_handle:%x,handle:%x, status:%x\r\n",
                SLE_RCU_SERVER_LOG, server_id, service_handle, handle, status);
}

static void ssaps_add_descriptor_cbk(uint8_t server_id, sle_uuid_t *uuid, uint16_t service_handle,
                                     uint16_t property_handle, errcode_t status)
{
    unused(uuid);
    osal_printk("%s add descriptor cbk callback server_id:%x, service_handle:%x, property_handle:%x, \
                status:%x\r\n", SLE_RCU_SERVER_LOG, server_id, service_handle, property_handle, status);
}

static void ssaps_delete_all_service_cbk(uint8_t server_id, errcode_t status)
{
    osal_printk("%s delete all service callback server_id:%x, status:%x\r\n", SLE_RCU_SERVER_LOG,
                server_id, status);
}

static errcode_t sle_ssaps_register_cbks(ssaps_read_request_callback ssaps_read_callback,
                                         ssaps_write_request_callback ssaps_write_callback)
{
    errcode_t ret;
    ssaps_callbacks_t ssaps_cbk = { 0 };
    ssaps_cbk.add_service_cb = ssaps_add_service_cbk;
    ssaps_cbk.add_property_cb = ssaps_add_property_cbk;
    ssaps_cbk.add_descriptor_cb = ssaps_add_descriptor_cbk;
    ssaps_cbk.start_service_cb = ssaps_start_service_cbk;
    ssaps_cbk.delete_all_service_cb = ssaps_delete_all_service_cbk;
    ssaps_cbk.mtu_changed_cb = ssaps_mtu_changed_cbk;
    ssaps_cbk.read_request_cb = ssaps_read_callback;
    ssaps_cbk.write_request_cb = ssaps_write_callback;
    ret = ssaps_register_callbacks(&ssaps_cbk);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_ssaps_register_cbks,ssaps_register_callbacks fail :%x\r\n", SLE_RCU_SERVER_LOG,
                    ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_uuid_server_service_add(void)
{
    errcode_t ret;
    sle_uuid_t service_uuid = { 0 };
    sle_uuid_setu2(SLE_UUID_SERVER_SERVICE, &service_uuid);
    ret = ssaps_add_service_sync(g_server_id, &service_uuid, 1, &g_service_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_uuid_server_property_add(void)
{
    errcode_t ret;
    ssaps_property_info_t property = { 0 };
    ssaps_desc_info_t descriptor = { 0 };
    uint8_t ntf_value[] = { 0x01, 0x00 };

    property.permissions = SLE_UUID_TEST_PROPERTIES;
    property.operate_indication = SLE_UUID_TEST_OPERATION_INDICATION;
    sle_uuid_setu2(SLE_UUID_SERVER_NTF_REPORT, &property.uuid);
    property.value = (uint8_t *)osal_vmalloc(sizeof(g_sle_property_value));
    if (property.value == NULL) {
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(property.value, sizeof(g_sle_property_value), g_sle_property_value,
        sizeof(g_sle_property_value)) != EOK) {
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }
    ret = ssaps_add_property_sync(g_server_id, g_service_handle, &property,  &g_property_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }
    descriptor.permissions = SLE_UUID_TEST_DESCRIPTOR;
    descriptor.type = SSAP_DESCRIPTOR_CLIENT_CONFIGURATION;
    descriptor.operate_indication = SLE_UUID_TEST_OPERATION_INDICATION;
    descriptor.value = (uint8_t *)osal_vmalloc(sizeof(ntf_value));
    if (descriptor.value == NULL) {
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(descriptor.value, sizeof(ntf_value), ntf_value, sizeof(ntf_value)) != EOK) {
        osal_vfree(property.value);
        osal_vfree(descriptor.value);
        return ERRCODE_SLE_FAIL;
    }
    ret = ssaps_add_descriptor_sync(g_server_id, g_service_handle, g_property_handle, &descriptor);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_vfree(property.value);
        osal_vfree(descriptor.value);
        return ERRCODE_SLE_FAIL;
    }
    osal_vfree(property.value);
    osal_vfree(descriptor.value);
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_uuid_amic_server_service_add(void)
{
    errcode_t ret;
    sle_uuid_t service_uuid = { 0 };
    sle_uuid_setu2(SLE_UUID_AMIC_SERVER_SERVICE, &service_uuid);
    ret = ssaps_add_service_sync(g_server_id, &service_uuid, 1, &g_service_amic_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_uuid_amic_server_property_add(void)
{
    errcode_t ret;
    ssaps_property_info_t property = { 0 };
    ssaps_desc_info_t descriptor = { 0 };
    uint8_t ntf_value[] = { 0x01, 0x00 };

    property.permissions = SLE_UUID_TEST_PROPERTIES;
    property.operate_indication = SLE_UUID_TEST_OPERATION_INDICATION;
    sle_uuid_setu2(SLE_UUID_AMIC_SERVER_NTF_REPORT, &property.uuid);
    property.value = (uint8_t *)osal_vmalloc(sizeof(g_sle_property_value));
    if (property.value == NULL) {
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(property.value, sizeof(g_sle_property_value), g_sle_property_value,
        sizeof(g_sle_property_value)) != EOK) {
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }
    ret = ssaps_add_property_sync(g_server_id, g_service_amic_handle, &property,  &g_amic_property_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }
    descriptor.permissions = SLE_UUID_TEST_DESCRIPTOR;
    descriptor.type = SSAP_DESCRIPTOR_CLIENT_CONFIGURATION;
    descriptor.operate_indication = SLE_UUID_TEST_OPERATION_INDICATION;
    descriptor.value = (uint8_t *)osal_vmalloc(sizeof(ntf_value));
    if (descriptor.value == NULL) {
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(descriptor.value, sizeof(ntf_value), ntf_value, sizeof(ntf_value)) != EOK) {
        osal_vfree(property.value);
        osal_vfree(descriptor.value);
        return ERRCODE_SLE_FAIL;
    }
    ret = ssaps_add_descriptor_sync(g_server_id, g_service_amic_handle, g_amic_property_handle, &descriptor);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_vfree(property.value);
        osal_vfree(descriptor.value);
        return ERRCODE_SLE_FAIL;
    }
    osal_vfree(property.value);
    osal_vfree(descriptor.value);
    return ERRCODE_SLE_SUCCESS;
}

static void sle_connect_state_changed_cbk(uint16_t conn_id, const sle_addr_t *addr, sle_acb_state_t conn_state,
                                          sle_pair_state_t pair_state, sle_disc_reason_t disc_reason)
{
    osal_printk("%s connect state changed callback conn_id:0x%02x, conn_state:0x%x, pair_state:0x%x, \
                disc_reason:0x%x\r\n", SLE_RCU_SERVER_LOG, conn_id, conn_state, pair_state, disc_reason);
    osal_printk("%s connect state changed callback addr:%02x:**:**:**:%02x:%02x\r\n", SLE_RCU_SERVER_LOG,
                addr->addr[BT_INDEX_0], addr->addr[BT_INDEX_4]);
    uint8_t mode;
    mode = get_rcu_mode();
    if (conn_state == SLE_ACB_STATE_CONNECTED) {
        g_sle_conn_id = conn_id;
        set_sle_work_conn_id(conn_id);
        g_sle_conn_handle[conn_id] = 1;
        g_sle_conn_num++;
        set_app_sle_conn_status(conn_id, APP_CONNECT_STATUS_CONNECTED);
        if (g_sle_conn_num < CONFIG_SLE_MULTICON_NUM) {
            sle_start_announce(SLE_ADV_HANDLE_DEFAULT);
        }
        memcpy_s(&g_sle_addr, sizeof(sle_addr_t), addr, sizeof(sle_addr_t));
        rcu_easy_connect_get_conn_id(conn_id);
    } else if (conn_state == SLE_ACB_STATE_DISCONNECTED) {
        g_sle_conn_handle[conn_id] = 0;
        g_sle_pair_handle = 0;
        g_sle_conn_num--;
        g_ssaps_ready = false;
        set_app_sle_conn_status(conn_id, APP_CONNECT_STATUS_DISCONNECT);
        memset_s(&g_sle_addr, sizeof(sle_addr_t), 0, sizeof(sle_addr_t));
        if (!g_low_power_state) {
            if (mode == RCU_MODE_ADV_SEND) {
                sle_start_announce(SLE_ADV_HANDLE_DEFAULT);
            }
        }
    }
}

uint8_t *sle_low_latency_get_data_cbk(uint8_t *length, uint16_t *ssap_handle, uint8_t *data_type, uint16_t co_handle)
{
    unused(data_type);
    unused(co_handle);

    // buffer空
    if (g_read_buffer_node == g_write_buffer_node) {
        *length = 0;
        return NULL;
    }

    uint8_t *out_data1, *out_data2;
    uint32_t buffer_filled_count = 0;
    for (uint32_t i = 0; i < CONFIG_USB_PDM_TRANSFER_LEN_BY_DMA; i++) {
        g_sle_pdm_buffer[buffer_filled_count++] =
            (uint8_t)(g_pdm_dma_data[g_read_buffer_node][i] >> SLE_VDT_MIC_OFFSET_16);
        g_sle_pdm_buffer[buffer_filled_count++] =
            (uint8_t)(g_pdm_dma_data[g_read_buffer_node][i] >> SLE_VDT_MIC_OFFSET_24);
    }

    g_read_buffer_node = (g_read_buffer_node + 1) % RING_BUFFER_NUMBER;
    buffer_filled_count = 0;
    uint32_t encode_data_len1 = sle_vdt_codec_encode(g_sle_pdm_buffer, &out_data1);
    uint32_t encode_data_len2 = sle_vdt_codec_encode(g_sle_pdm_buffer + CONFIG_USB_PDM_TRANSFER_LEN_BY_DMA, &out_data2);

    if (memcpy_s(g_out_low_latency_data, LOW_LATENCY_DATA_MAX, out_data1, encode_data_len1) != EOK) {
        osal_printk("memcpy first part data fail.\r\n");
    }
    if (memcpy_s(g_out_low_latency_data + encode_data_len1, LOW_LATENCY_DATA_MAX - encode_data_len1, out_data2,
        encode_data_len2) != EOK) {
        osal_printk("memcpy second part data fail.\r\n");
    }

    *ssap_handle = SLE_RCU_SSAP_RPT_HANDLE;
    *length = encode_data_len1 + encode_data_len2;
    return g_out_low_latency_data;
}

void sle_set_em_data_cbk(uint16_t co_handle, uint8_t status)
{
    unused(status);
    unused(co_handle);

    osal_printk("pair sle_set_em_data_cbk\r\n");
}

void sle_low_latency_cbk_reg(void)
{
    sle_low_latency_callbacks_t cbks = {0};
    cbks.hid_data_cb = sle_low_latency_get_data_cbk;
    cbks.sle_set_em_data_cb = sle_set_em_data_cbk;
    sle_low_latency_register_callbacks(&cbks);
}

static void sle_pair_complete_cbk(uint16_t conn_id, const sle_addr_t *addr, errcode_t status)
{
    unused(conn_id);
    unused(addr);
    unused(status);
    osal_printk("pair complete\r\n");
    g_sle_pair_handle = conn_id + 1;
    set_app_sle_conn_status(conn_id, APP_CONNECT_STATUS_PAIRED);
    sle_low_latency_cbk_reg();
}

void sle_connect_param_update_cb(uint16_t conn_id, errcode_t status, const sle_connection_param_update_evt_t *param)
{
    unused(conn_id);
    unused(status);
    unused(param);
    osal_printk("sle_connect_param_update_cb interval_min %d*0.125 ms\r\n", param->interval);
    g_conn_update = 1;
}

static errcode_t sle_conn_register_cbks(void)
{
    errcode_t ret;
    sle_connection_callbacks_t conn_cbks = { 0 };
    conn_cbks.connect_state_changed_cb = sle_connect_state_changed_cbk;
    conn_cbks.pair_complete_cb = sle_pair_complete_cbk;
    conn_cbks.connect_param_update_cb = sle_connect_param_update_cb;
    ret = sle_connection_register_callbacks(&conn_cbks);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_conn_register_cbks,sle_connection_register_callbacks fail :%x\r\n",
                    SLE_RCU_SERVER_LOG, ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}

uint16_t sle_rcu_client_is_connected(void)
{
    return g_sle_pair_handle;
}

void sle_rcu_work_to_standby(void)
{
    if (g_sle_conn_num > 0) {
        g_low_power_state = true;
        g_work_to_standby.conn_id = g_sle_conn_id;
        sle_update_connect_param(&g_work_to_standby);
    }
}

void sle_rcu_standby_to_work(void)
{
    if (g_sle_conn_num > 0) {
        g_standby_to_work.conn_id = g_sle_conn_id;
        sle_update_connect_param(&g_standby_to_work);
    }
    g_low_power_state = false;
}

void sle_rcu_standby_to_sleep(void)
{
    if (g_sle_conn_num > 0) {
        g_low_power_state = true;
        sle_disconnect_remote_device(&g_sle_addr);
    } else if (g_sle_conn_num == 0) {
        sle_stop_announce(SLE_ADV_HANDLE_DEFAULT);
    }
}

void sle_rcu_sleep_to_work(void)
{
    sle_start_announce(SLE_ADV_HANDLE_DEFAULT);
    g_low_power_state = false;
}

static errcode_t sle_add_services(void)
{
    sle_set_server_id(g_server_id);

    // add hid service
    uint8_t sle_input_report[SLE_INPUT_REPORT_LENGTH] = {0};
    uint8_t input_report_descriptor[SLE_SRV_ENCODED_REPORT_LEN] = {0};
    sle_service_hids_t hid_service = { 0 };
    hid_service.hid_control_point = SLE_HID_POINT;
    hid_service.cccd = g_cccd;
    hid_service.cccd_len = sizeof(g_cccd);
    hid_service.input_report = sle_input_report;
    hid_service.input_report_descriptor = input_report_descriptor;
    hid_service.report_map_datas = g_sle_report_map_datas;
    hid_service.map_data_len = sizeof(g_sle_report_map_datas);
    hid_service.item_handle = g_service_hdl;
    if (ERRCODE_SLE_SUCCESS != sle_rcu_hid_service_add(&hid_service)) {
        osal_printk("%s sle_rcu_server_init,sle_rcu_hid_service_add fail\r\n", SLE_RCU_SERVER_LOG);
        return ERRCODE_SLE_FAIL;
    }

    // add dis service
    if (ERRCODE_SLE_SUCCESS != sle_rcu_dis_service_add()) {
        osal_printk("%s sle_rcu_server_init,sle_rcu_dis_service_add fail\r\n", SLE_RCU_SERVER_LOG);
        return ERRCODE_SLE_FAIL;
    }

    // add bas service
    uint8_t channel = 1;
    bool self_cali = true;
    adc_port_gadc_entirely_open(channel, self_cali);
    sle_set_battert(adc_port_gadc_entirely_sample(channel));
    if (ERRCODE_SLE_SUCCESS != sle_rcu_bas_service_add()) {
        osal_printk("%s sle_rcu_server_init,sle_rcu_bas_service_add fail\r\n", SLE_RCU_SERVER_LOG);
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_rcu_server_add(void)
{
    errcode_t ret;
    sle_uuid_t app_uuid = { 0 };

    app_uuid.len = sizeof(g_sle_uuid_app_uuid);
    if (memcpy_s(app_uuid.uuid, app_uuid.len, g_sle_uuid_app_uuid, sizeof(g_sle_uuid_app_uuid)) != EOK) {
        return ERRCODE_SLE_FAIL;
    }
    ssaps_register_server(&app_uuid, &g_server_id);
    rcu_easy_connect_get_server_id(g_server_id);
    sle_ota_service_init(g_server_id);
    if (sle_uuid_server_service_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_server_id);
        return ERRCODE_SLE_FAIL;
    }
    if (sle_uuid_server_property_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_server_id);
        return ERRCODE_SLE_FAIL;
    }
    ret = ssaps_start_service(g_server_id, g_service_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        return ERRCODE_SLE_FAIL;
    }
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[1]%s sle rcu add service fail, ret:%x\r\n", SLE_RCU_SERVER_LOG, ret);
        return ERRCODE_SLE_FAIL;
    }
    // add hids, bas, dis services;
    if (sle_add_services() != ERRCODE_SLE_SUCCESS) {
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_rcu_amic_server_add(void)
{
    errcode_t ret;
    if (sle_uuid_amic_server_service_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_server_id);
        return ERRCODE_SLE_FAIL;
    }
    if (sle_uuid_amic_server_property_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_server_id);
        return ERRCODE_SLE_FAIL;
    }
    osal_printk("%s sle rcu add service, server_id:%x, service_handle:%x, property_handle:%x\r\n",
                SLE_RCU_SERVER_LOG, g_server_id, g_service_amic_handle, g_amic_property_handle);
    ret = ssaps_start_service(g_server_id, g_service_amic_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

/* device通过uuid向host发送数据：report */
errcode_t sle_rcu_server_send_report_by_uuid(const uint8_t *data, uint8_t len, uint16_t conn_id)
{
    osal_printk("enter sle rcu server send report by uuid function! \n");
    errcode_t ret;
    ssaps_ntf_ind_by_uuid_t param = { 0 };
    param.type = SSAP_PROPERTY_TYPE_VALUE;
    param.start_handle = g_service_handle;
    param.end_handle = g_property_handle;
    param.value_len = len;
    param.value = (uint8_t *)osal_vmalloc(len);
    if (param.value == NULL) {
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(param.value, param.value_len, data, len) != EOK) {
        osal_vfree(param.value);
        return ERRCODE_SLE_FAIL;
    }
    sle_uuid_setu2(SLE_UUID_SERVER_NTF_REPORT, &param.uuid);
    ret = ssaps_notify_indicate_by_uuid(g_server_id, conn_id, &param);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_rcu_server_send_report_by_uuid,ssaps_notify_indicate_by_uuid fail :%x\r\n",
                    SLE_RCU_SERVER_LOG, ret);
        osal_vfree(param.value);
        return ret;
    }
    osal_vfree(param.value);
    return ERRCODE_SLE_SUCCESS;
}

/* device通过handle向host发送数据：report */
errcode_t sle_rcu_server_send_report_by_handle(const uint8_t *data, uint8_t len, uint16_t conn_id)
{
    ssaps_ntf_ind_t param = { 0 };
    uint8_t receive_buf[RCU_SEND_BUFF_LENGTH] = { 0 }; /* max receive length. */
    param.handle = g_service_hdl[SLE_HID_INDEX_KEYBOARD_INPUT].handle_out;
    param.type = SSAP_PROPERTY_TYPE_VALUE;
    param.value = receive_buf;
    param.value_len = len;
    if (memcpy_s(param.value, param.value_len, data, len) != EOK) {
        return ERRCODE_SLE_FAIL;
    }
    return ssaps_notify_indicate(g_server_id, conn_id, &param);
}

errcode_t sle_rcu_amic_server_send_report_by_handle(uint8_t *data, uint8_t len, uint16_t conn_id)
{
    ssaps_ntf_ind_t param = { 0 };
    param.handle = g_amic_property_handle;
    param.type = SSAP_PROPERTY_TYPE_VALUE;
    param.value = data;
    param.value_len = len;
    ssaps_notify_indicate(g_server_id, conn_id, &param);
    return ERRCODE_SLE_SUCCESS;
}


/* 初始化uuid server */
errcode_t sle_rcu_server_init(ssaps_read_request_callback ssaps_read_callback,
                              ssaps_write_request_callback ssaps_write_callback)
{
    errcode_t ret;
    ret = sle_rcu_announce_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS) {
        return ret;
    }
    ret = sle_conn_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS) {
        return ret;
    }
    ret = sle_ssaps_register_cbks(ssaps_read_callback, ssaps_write_callback);
    if (ret != ERRCODE_SLE_SUCCESS) {
        return ret;
    }
    ret = enable_sle();
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_rcu_server_init,enable_sle fail :%x\r\n", SLE_RCU_SERVER_LOG, ret);
        return ret;
    }
    osal_printk("%s init ok\r\n", SLE_RCU_SERVER_LOG);
    return ERRCODE_SLE_SUCCESS;
}