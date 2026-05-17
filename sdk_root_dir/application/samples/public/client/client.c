/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: client source. \n
 * Author: Triductor \n
 * History: \n
 * 2023-04-03, Create file. \n
 */
#include "string.h"
#include "common_def.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "sle_errcode.h"
#include "cmsis_os2.h"
#include "securec.h"
#include "tcxo.h"
#include "bts_le_gap.h"
#include "sle_device_discovery.h"
#include "sle_device_manager.h"
#include "sle_connection_manager.h"
#include "sle_ssap_client.h"
#include "../public_init.h"
#include "../msg/msg.h"
#include "key_id.h"
#include "common.h"
#include "nv.h"
#include "osal_addr.h"
#include "client.h"

#define BT_INDEX_4     4
#define BT_INDEX_5     5
#define BT_INDEX_1     1
#define BT_INDEX_2     2
#define BT_INDEX_3     3
#define BT_INDEX_0     0
#define SLE_MTU_SIZE_DEFAULT 520
#define SLE_SEEK_INTERVAL_DEFAULT 50
#define SLE_SEEK_WINDOW_DEFAULT 50
#define UUID_16BIT_LEN 2
#define UUID_128BIT_LEN 16

#define SLE_CLIENT_LOG                     "[sle client]"

static sle_dev_manager_callbacks_t g_sle_dev_mgr_cbk = { 0 };
static sle_announce_seek_callbacks_t g_sle_client_seek_cbk = { 0 };
static sle_connection_callbacks_t g_sle_client_connect_cbk = { 0 };
static ssapc_callbacks_t g_sle_client_ssapc_cbk = { 0 };
static sle_addr_t g_sle_client_own_addr = {0x00, {0x00, 0x00, 0x00, 0x0a, 0x0b, 0x0c} };

static void sle_enable_cb(uint8_t status)
{
    osal_printk("%s sle enable status: %d\r\n", SLE_CLIENT_LOG, status);
    if(status != ERRCODE_SLE_SUCCESS){
        return;
    }
    app_message_t msg_node = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_ENABLE_COMPLETE;
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_client_power_on_cbk(uint8_t status)
{
    osal_printk("%s sle power on: %d.\r\n", SLE_CLIENT_LOG, status);
    if(status != ERRCODE_SLE_SUCCESS){
        return;
    }
    app_message_t msg_node = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_POWER_ON_COMPLETE;
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_client_seek_enable_cbk(errcode_t status)
{
    osal_printk("%s seek_enable,status =%x\r\n", SLE_CLIENT_LOG, status);
    if (status != ERRCODE_SUCC) {
        return;
    }
    sle_set_local_addr(&g_sle_client_own_addr);
    app_message_t msg_node = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_CLIENT_SEEK_ENABLE;
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_client_seek_disable_cbk(errcode_t status)
{
    osal_printk("%s seek_disable,status =%x\r\n", SLE_CLIENT_LOG, status);
    if (status != ERRCODE_SUCC) {
        app_message_t msg_node = { 0 };
        msg_node.msg_type = APP_MSG_TYPE_SLE;
        msg_node.sub_type = APP_SLE_CLIENT_SEEK_DISABLE_ERROR; // todo
	    sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
        return;
    }
    app_message_t msg_node = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_CLIENT_SEEK_DISABLE;
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_client_seek_result_info_cbk(sle_seek_result_info_t *seek_result_data)
{
    if (seek_result_data == NULL || seek_result_data->data == NULL) {
        osal_printk("%s seek_result error\r\n", SLE_CLIENT_LOG);
    } else {
#ifdef CONFIG_SAMPLE_SUPPORT_MOUSE_DONGLE
        extern bool mouse_sle_client_seek_result(int8_t rssi, uint8_t *addr, uint8_t data_length, uint8_t *data);
        if(mouse_sle_client_seek_result(seek_result_data->rssi, seek_result_data->addr.addr, seek_result_data->data_length, seek_result_data->data) == true) {
            return;
        }
#endif
        app_message_t msg_node = { 0 };
        seek_result_message_t seek_result_msg = { 0 };
        msg_node.msg_type = APP_MSG_TYPE_SLE;
        msg_node.sub_type = APP_SLE_CLIENT_SEEK_COMPLETE;
        msg_node.length = sizeof(seek_result_message_t);
        seek_result_msg.rssi = seek_result_data->rssi;
        seek_result_msg.data_length = seek_result_data->data_length;
        (void)memcpy_s(seek_result_msg.addr, SLE_ADDR_SIZE, seek_result_data->addr.addr, SLE_ADDR_SIZE); 
        (void)memcpy_s(seek_result_msg.data, seek_result_data->data_length, seek_result_data->data, seek_result_data->data_length); 
        (void)memcpy_s(msg_node.data, sizeof(seek_result_message_t), &seek_result_msg, sizeof(seek_result_message_t));
        sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
    }
}

static void sle_client_connect_state_changed_cbk(uint16_t conn_id, const sle_addr_t *addr,
                                                             sle_acb_state_t conn_state, sle_pair_state_t pair_state,
                                                             sle_disc_reason_t disc_reason)
{
    osal_printk("%s connect state changed conn_id:0x%02x, conn_state:0x%x, pair_state:0x%x, \
        disc_reason:0x%x\r\n", SLE_CLIENT_LOG, conn_id, conn_state, pair_state, disc_reason);
    app_message_t msg_node = { 0 };
    connect_paire_message_t con_msg_t = { 0 };
    if (conn_state == SLE_ACB_STATE_CONNECTED) {
        msg_node.msg_type = APP_MSG_TYPE_SLE;
        msg_node.sub_type = APP_SLE_CONNECT_SUCCESS;
        msg_node.length = sizeof(connect_paire_message_t);
        con_msg_t.conn_id = conn_id;
        (void)memcpy_s(&(con_msg_t.addr), sizeof(sle_addr_t), addr, sizeof(sle_addr_t)); 
        (void)memcpy_s(msg_node.data, sizeof(connect_paire_message_t), &con_msg_t, sizeof(connect_paire_message_t));
        sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
    } else if (conn_state == SLE_ACB_STATE_DISCONNECTED) {
        msg_node.msg_type = APP_MSG_TYPE_SLE;
        msg_node.sub_type = APP_SLE_DISCONNECT_SUCCESS;
        msg_node.length = sizeof(connect_paire_message_t);
        con_msg_t.conn_id = conn_id;
        (void)memcpy_s(&(con_msg_t.addr), sizeof(sle_addr_t), addr, sizeof(sle_addr_t)); 
        (void)memcpy_s(msg_node.data, sizeof(connect_paire_message_t), &con_msg_t, sizeof(connect_paire_message_t));
        sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
    }
}

static void sle_client_pair_complete_cbk(uint16_t conn_id, const sle_addr_t *addr, errcode_t status)
{
    osal_printk("%s pair complete conn_id:%02x, status:%x\r\n", SLE_CLIENT_LOG,
                conn_id, status);
    osal_printk("%s pair complete addr:%02x:**:**:**:%02x:%02x\r\n", SLE_CLIENT_LOG,
                addr->addr[BT_INDEX_0], addr->addr[BT_INDEX_4], addr->addr[BT_INDEX_5]);
    if(status != ERRCODE_SLE_SUCCESS){
        return;
    }
    connect_paire_message_t paire_msg_t = { 0 };
    app_message_t msg_node = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_PAIRE_COMPLETE;
    msg_node.length = sizeof(connect_paire_message_t);
    paire_msg_t.conn_id = conn_id;
    (void)memcpy_s(&(paire_msg_t.addr), sizeof(sle_addr_t), addr, sizeof(sle_addr_t)); 
    (void)memcpy_s(msg_node.data, sizeof(connect_paire_message_t), &paire_msg_t, sizeof(connect_paire_message_t));
    sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_client_connect_param_update_cb(uint16_t conn_id, errcode_t status, const sle_connection_param_update_evt_t *param)
{
    unused(conn_id);
    unused(status);
    unused(param);
    osal_printk("%s interval %d*0.125ms latency %d supervision %d*10ms, status=%x\r\n", SLE_CLIENT_LOG, param->interval, param->latency, param->supervision, status);
    if(status != ERRCODE_SLE_SUCCESS) {
        return;
    }
    app_message_t msg_node = { 0 };
    interval_update_message_t interv_update_msg = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_INTERVAL_UPDATE_SUCCESS;
    msg_node.length = sizeof(interval_update_message_t);
    interv_update_msg.conn_id = conn_id;
    interv_update_msg.inerval = param->interval;
    (void)memcpy_s(msg_node.data, sizeof(interval_update_message_t), &interv_update_msg, sizeof(interval_update_message_t));
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_client_phy_update_cb(uint16_t conn_id, errcode_t status, const sle_set_phy_t *param)
{
    unused(conn_id);
    unused(param);
    osal_printk("%s sle_phy_update_cb status %x\r\n", SLE_CLIENT_LOG, status);
    if(status != ERRCODE_SLE_SUCCESS){
        return;
    }
    app_message_t msg_node = { 0 };
    phy_update_message_t phy_update_msg = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_UPDATE_PHY_SUCCESS;
    msg_node.length = sizeof(phy_update_message_t);
    phy_update_msg.conn_id = conn_id;
    (void)memcpy_s(&phy_update_msg.sle_phy_param, sizeof(sle_set_phy_t), param, sizeof(sle_set_phy_t));
    (void)memcpy_s(msg_node.data, sizeof(phy_update_message_t), &phy_update_msg, sizeof(phy_update_message_t));
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_low_latency_cb(uint8_t status, sle_addr_t *addr, uint8_t rate)
{
    unused(addr);
    
    osal_printk("%s sle_low_latency_cb status %x rate %d\r\n", SLE_CLIENT_LOG, status, rate);
}

static void sle_client_exchange_info_cbk(uint8_t client_id, uint16_t conn_id, ssap_exchange_info_t *param,
                                                     errcode_t status)
{
    osal_printk("%s exchange_info,pair complete client id:%d status:%x\r\n",
                SLE_CLIENT_LOG, client_id, status);
    osal_printk("%s exchange mtu, mtu size: %d, version: %d.\r\n", SLE_CLIENT_LOG,
                param->mtu_size, param->version);
    if(status != ERRCODE_SLE_SUCCESS){
        return;
    }
    app_message_t msg_node = { 0 };
    mtu_message_t mtu_msg_t = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_MTU_EXCHANGE_SUCCESS;
    msg_node.length = sizeof(mtu_message_t);
    mtu_msg_t.conn_id = conn_id;
    mtu_msg_t.mtu_size = param->mtu_size;
    (void)memcpy_s(msg_node.data, sizeof(mtu_message_t), &mtu_msg_t, sizeof(mtu_message_t));
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_client_find_structure_cbk(uint8_t client_id, uint16_t conn_id,
                                                   ssapc_find_service_result_t *service,
                                                   errcode_t status)
{
    if(status != ERRCODE_SLE_SUCCESS || service == NULL){
        return;
    }
    osal_printk("%s find structure client: %d conn_id:%d status: 0x%x \r\n", SLE_CLIENT_LOG,
                client_id, conn_id, status);
    osal_printk("%s find structure start_hdl:[0x%x], end_hdl:[0x%x], uuid len:%d\r\n", SLE_CLIENT_LOG,
                service->start_hdl, service->end_hdl, service->uuid.len);
    app_message_t msg_node = { 0 };
    service_result_message_t property_result_msg_t = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_FIND_STRUCT_SUCCESS;
    msg_node.length = sizeof(property_result_message_t);
    property_result_msg_t.conn_id = conn_id;
    (void)memcpy_s(&property_result_msg_t.service_result, sizeof(ssapc_find_service_result_t), service, sizeof(ssapc_find_service_result_t));
    (void)memcpy_s(msg_node.data, sizeof(service_result_message_t), &property_result_msg_t, sizeof(service_result_message_t));
    sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_client_find_property_cbk(uint8_t client_id, uint16_t conn_id,
                                                    ssapc_find_property_result_t *property, errcode_t status)
{
    if(status != ERRCODE_SLE_SUCCESS || property == NULL){
        return;
    }
    unused(client_id);
    app_message_t msg_node = { 0 };
    property_result_message_t property_result_msg_t = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_FIND_PROPERTY_SUCCESS;
    msg_node.length = sizeof(property_result_message_t);
    property_result_msg_t.conn_id = conn_id;
    property_result_msg_t.handle = property->handle;
    (void)memcpy_s(&property_result_msg_t.uuid, sizeof(sle_uuid_t), &property->uuid, sizeof(sle_uuid_t));
    (void)memcpy_s(msg_node.data, sizeof(property_result_message_t), &property_result_msg_t, sizeof(property_result_message_t));
    sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_client_find_structure_cmp_cbk(uint8_t client_id, uint16_t conn_id,
                                                          ssapc_find_structure_result_t *structure_result,
                                                          errcode_t status)
{
    if(status != ERRCODE_SLE_SUCCESS || structure_result == NULL){
        return;
    }
    osal_printk("%s find_structure_cmp,client id:%d status:%d type:%d uuid len:%d \r\n",
                SLE_CLIENT_LOG, client_id, status, structure_result->type, structure_result->uuid.len);
    app_message_t msg_node = { 0 };
    structor_result_message_t struc_msg_t = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_FIND_STRUCT_CMP_SUCCESS;
    msg_node.length = sizeof(structor_result_message_t);
    struc_msg_t.conn_id = conn_id;
    (void)memcpy_s(&struc_msg_t.sle_structure_result, sizeof(ssapc_find_structure_result_t),structure_result, sizeof(ssapc_find_structure_result_t));
    (void)memcpy_s(msg_node.data, sizeof(structor_result_message_t), &struc_msg_t, sizeof(structor_result_message_t));
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static errcode_t sle_client_start_seek(uint16_t seek_interval)
{
    errcode_t ret;
    sle_seek_param_t param = { 0 };
    param.filter_duplicates = ENABLE_DUMPLICATES_FILTER;
    param.seek_filter_policy = SLE_SEEK_FILTER_ALLOW_ALL;
    param.seek_phys = SLE_SEEK_PHY_1M;
    param.seek_type[0] = SLE_SEEK_ACTIVE;
    param.seek_interval[0] = seek_interval;
    param.seek_window[0] = SLE_SEEK_WINDOW_DEFAULT;
    ret = sle_set_seek_param(&param);
    if(ret != ERRCODE_SUCC){
        return ret;
    }
    ret = sle_start_seek();
    osal_printk("%s sle_client_start_seek, ret=%x\r\n", SLE_CLIENT_LOG, ret);
    return ret;
}

static errcode_t sle_client_config_own_addr(sle_addr_t *addr)
{
    if(addr == NULL) {
        return ERRCODE_FAIL;
    }

    (void)memcpy_s(g_sle_client_own_addr.addr, SLE_ADDR_SIZE, addr->addr, SLE_ADDR_SIZE);

    osal_printk("sle client own addr: ");
    for(uint8_t i =0; i< SLE_ADDR_SIZE;i++){
        osal_printk("0x%02x ", g_sle_client_own_addr.addr[i]);
    }
    osal_printk("\r\n");
    osal_printk("%s sle_client_config_own_addr\r\n", SLE_CLIENT_LOG);
    return ERRCODE_SUCC;
}

static errcode_t sle_client_stop_seek(void)
{
    errcode_t ret = sle_stop_seek();
    osal_printk("%s sle_client_stop_seek, ret=%x\r\n", SLE_CLIENT_LOG, ret);
    return ret;
}

static errcode_t sle_client_start_connect(sle_addr_t *addr)
{
    errcode_t ret = ERRCODE_FAIL;
    if(addr == NULL) {
        return ret;
    }
    ret = sle_connect_remote_device(addr);
    osal_printk("%s sle_client_start_connect, ret=%x\r\n", SLE_CLIENT_LOG, ret);
    return ret;
}

static errcode_t sle_client_start_disconnect(sle_addr_t *addr)
{
    errcode_t ret = ERRCODE_FAIL;
    if(addr == NULL) {
        return ret;
    }
    ret = sle_disconnect_remote_device(addr);
    osal_printk("%s sle_client_start_disconnect, ret=%x\r\n", SLE_CLIENT_LOG, ret);
    return ret;
}

static errcode_t sle_client_start_paire(sle_addr_t *addr)
{
    errcode_t ret = ERRCODE_FAIL;
    if(addr == NULL) {
        return ret;
    }
    ret = sle_pair_remote_device(addr);
    osal_printk("%s sle_client_start_paire, ret=%x\r\n", SLE_CLIENT_LOG, ret);
    return ret;
}

static errcode_t sle_client_remove_paire(sle_addr_t *addr)
{
    errcode_t ret = ERRCODE_FAIL;
    if(addr == NULL) {
        return ret;
    }
    ret = sle_remove_paired_remote_device(addr);
    osal_printk("%s sle_client_remove_paire, ret=%x\r\n", SLE_CLIENT_LOG, ret);
    return ret;
}

static errcode_t sle_client_find_struct(uint16_t conn_id)
{
    errcode_t ret = ERRCODE_FAIL;
    ssapc_find_structure_param_t find_param = { 0 };
    find_param.type = SSAP_FIND_TYPE_PROPERTY;
    find_param.start_hdl = 1;
    find_param.end_hdl = 0xFFFF;
    ret = ssapc_find_structure(SLE_CLIENT_ID, conn_id, &find_param);
    osal_printk("%s sle_client_find_struct, ret=%x\r\n", SLE_CLIENT_LOG, ret);
    return ret;
}

static errcode_t sle_client_start_mtu_exchange(mtu_param_t *mtu_str)
{
    if(mtu_str == NULL) {
        osal_printk("%s sle_client_start_mtu_exchange fail, mtu_str is NULL\r\n", SLE_CLIENT_LOG);
        return ERRCODE_FAIL;
    }
    errcode_t ret;
    ssap_exchange_info_t info = {0};
    info.mtu_size = mtu_str->mtu;
    info.version = VERSION_1;
    ret = ssapc_exchange_info_req(0, mtu_str->conn_id, &info);
    osal_printk("%s sle_client_start_mtu_exchange, ret=%x\r\n", SLE_CLIENT_LOG, ret);
    return ret;
}

static errcode_t sle_client_start_update_phy(phy_param_t *phy_param_str)
{
    if(phy_param_str == NULL) {
        osal_printk("%s sle_client_start_update_phy fail, phy_param_str is NULL\r\n", SLE_CLIENT_LOG);
        return ERRCODE_FAIL;
    }
    errcode_t ret;
    sle_set_phy_t param = {0};
    param.tx_format = phy_param_str->format;
    param.rx_format = phy_param_str->format;
    param.tx_phy = phy_param_str->phy;
    param.rx_phy = phy_param_str->phy;
    param.tx_pilot_density = SLE_PHY_PILOT_DENSITY_4_TO_1;
    param.rx_pilot_density = SLE_PHY_PILOT_DENSITY_4_TO_1;
    ret = sle_set_phy_param(phy_param_str->conn_id, &param);
    osal_printk("%s sle_client_start_update_phy, ret=%x\r\n", SLE_CLIENT_LOG, ret);
    return ret;
}

static errcode_t sle_client_start_update_interval(interval_param_t *interval_param_str)
{
    if(interval_param_str == NULL) {
        osal_printk("%s sle_client_start_update_interval fail, interval_param_str is NULL\r\n", SLE_CLIENT_LOG);
        return ERRCODE_FAIL;
    }
    errcode_t ret;
    sle_connection_param_update_t params = { 0 };
    params.conn_id = interval_param_str->conn_id;
    params.interval_min = interval_param_str->interval_min;
    params.interval_max = interval_param_str->interval_max;
    params.max_latency = interval_param_str->max_latency;
    params.supervision_timeout = interval_param_str->supervision_timeout;
    ret =  sle_update_connect_param(&params);
    osal_printk("%s sle_client_start_update_interval, ret=%x\r\n", SLE_CLIENT_LOG, ret);
    return ret;
}

static errcode_t sle_enable_sle(void)
{
    osal_printk("%s enter sle_enable_sle!\r\n", SLE_CLIENT_LOG);
    return enable_sle();
}

static errcode_t sle_disable_sle(void)
{
    osal_printk("%s enter sle_disable_sle!\r\n", SLE_CLIENT_LOG);
    return disable_sle();
}

static errcode_t sle_client_send_data(ssapc_send_param_t *ssapc_send_param_str)
{
    if(ssapc_send_param_str == NULL) {
        osal_printk("%s sle_client_send_data fail, ssapc_send_param_str is NULL\r\n", SLE_CLIENT_LOG);
        return ERRCODE_FAIL;
    }
    return ssapc_write_cmd(SLE_CLIENT_ID, ssapc_send_param_str->conn_id, &ssapc_send_param_str->ssapc_write_param);
}

static void sle_client_dev_cbk_register(void)
{
    g_sle_dev_mgr_cbk.sle_power_on_cb = sle_client_power_on_cbk;
    g_sle_dev_mgr_cbk.sle_enable_cb = sle_enable_cb;
    sle_dev_manager_register_callbacks(&g_sle_dev_mgr_cbk);

    enable_sle();
}

static void sle_client_seek_cbk_register(void)
{
    g_sle_client_seek_cbk.seek_enable_cb = sle_client_seek_enable_cbk;
    g_sle_client_seek_cbk.seek_result_cb = sle_client_seek_result_info_cbk;
    g_sle_client_seek_cbk.seek_disable_cb = sle_client_seek_disable_cbk;
    sle_announce_seek_register_callbacks(&g_sle_client_seek_cbk);
}

static void sle_client_connect_cbk_register(void)
{
    g_sle_client_connect_cbk.connect_state_changed_cb = sle_client_connect_state_changed_cbk;
    g_sle_client_connect_cbk.pair_complete_cb =sle_client_pair_complete_cbk;
    g_sle_client_connect_cbk.connect_param_update_cb = sle_client_connect_param_update_cb;
    g_sle_client_connect_cbk.set_phy_cb = sle_client_phy_update_cb;
    g_sle_client_connect_cbk.low_latency_cb = sle_low_latency_cb;
    sle_connection_register_callbacks(&g_sle_client_connect_cbk);
}

static void sle_client_ssapc_cbk_register(ssapc_notification_callback notification_cb,
                                                      ssapc_notification_callback indication_cb)
{
    g_sle_client_ssapc_cbk.exchange_info_cb = sle_client_exchange_info_cbk;
    g_sle_client_ssapc_cbk.find_structure_cb = sle_client_find_structure_cbk;
    g_sle_client_ssapc_cbk.ssapc_find_property_cbk = sle_client_find_property_cbk;
    g_sle_client_ssapc_cbk.find_structure_cmp_cb = sle_client_find_structure_cmp_cbk;
    g_sle_client_ssapc_cbk.notification_cb = notification_cb;
    g_sle_client_ssapc_cbk.indication_cb = indication_cb;
    ssapc_register_callbacks(&g_sle_client_ssapc_cbk);
}

void sle_client_state_func_register(sle_state_handler_t *handler)
{
    handler->sle_config_own_addr = sle_client_config_own_addr;
    handler->sle_start_seek = sle_client_start_seek;
    handler->sle_stop_seek = sle_client_stop_seek;
    handler->sle_start_connect = sle_client_start_connect;
    handler->sle_start_disconnect = sle_client_start_disconnect;
    handler->sle_start_paire = sle_client_start_paire;
    handler->sle_remove_paire = sle_client_remove_paire;
    handler->sle_start_find_struct = sle_client_find_struct;
    handler->sle_start_mtu_exchange = sle_client_start_mtu_exchange;
    handler->sle_start_interval_update = sle_client_start_update_interval;
    handler->sle_start_phy_update = sle_client_start_update_phy;
    handler->sle_enable_sle = sle_enable_sle;
    handler->sle_disable_sle = sle_disable_sle;
    handler->ssapc_send_data = sle_client_send_data;
}

void sle_client_init(ssapc_notification_callback notification_cb, ssapc_indication_callback indication_cb)
{
    sle_client_dev_cbk_register();
    sle_client_seek_cbk_register();
    sle_client_connect_cbk_register();
    sle_client_ssapc_cbk_register(notification_cb, indication_cb);
}
