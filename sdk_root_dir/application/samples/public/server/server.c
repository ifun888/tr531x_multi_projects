/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: SLE common Server Source. \n
 * Author: Triductor \n
 * History: \n
 * 2023-07-17, Create file. \n
 */

#include "securec.h"
#include "sle_common.h"
#include "common_def.h"
#include "osal_debug.h"
#include "sle_errcode.h"
#include "cmsis_os2.h"
#include "osal_addr.h"
#include "osal_task.h"
#include "../public_init.h"
#include "../msg/msg.h"
#include "sle_device_manager.h"
#include "sle_connection_manager.h"
#include "sle_device_discovery.h"
#include "server_adv.h"
#include "osal_addr.h"
#include "server.h"
#include "sle_transmition_manager.h"
#include "mouse_sle_hid_service.h"
#include "mouse_sle_dis_service.h"
#include "mouse_sle_charge_service.h"
#include "mouse_sle_dongle_usb_service.h"

#define BT_INDEX_4     4
#define BT_INDEX_5     5
#define BT_INDEX_0     0
#define SERVER_OWN_MTU 520

#define SLE_ADV_HANDLE_DEFAULT                    1
/* 广播ID */

/* sle server handle */
static uint8_t g_server_id = 0;

#define SLE_SERVER_LOG "[sle server]"

static void sle_trans_sig_cap_req(uint16_t conn_id);

static void ssaps_mtu_changed_cbk(uint8_t server_id, uint16_t conn_id,  ssap_exchange_info_t *mtu_size,
    errcode_t status)
{
    if (status != ERRCODE_SLE_SUCCESS || mtu_size == NULL){
        return;
    }
    osal_printk("%s server_id:0x%02x, conn_id:%d, mtu_size:%d, status:%x\r\n",
        SLE_SERVER_LOG, server_id, conn_id, mtu_size->mtu_size, status);
    app_message_t msg_node = { 0 };
    mtu_message_t mtu_msg_t = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_MTU_EXCHANGE_SUCCESS;
    msg_node.length = sizeof(mtu_message_t);
    mtu_msg_t.conn_id = conn_id;
    mtu_msg_t.mtu_size = mtu_size->mtu_size;
    (void)memcpy_s(msg_node.data, sizeof(mtu_message_t), &mtu_msg_t, sizeof(mtu_message_t));
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void ssaps_start_service_cbk(uint8_t server_id, uint16_t handle, errcode_t status)
{
    osal_printk("%s start service cbk callback server_id:%d, handle:%x, status:%x\r\n", SLE_SERVER_LOG,
        server_id, handle, status);
}
static void ssaps_add_service_cbk(uint8_t server_id, sle_uuid_t *uuid, uint16_t handle, errcode_t status)
{
    osal_printk("%s add service cbk callback server_id:%d, handle:%x, status:%x\r\n", SLE_SERVER_LOG,
        server_id, handle, status);
    sle_uuid_print(uuid);
}
static void ssaps_add_property_cbk(uint8_t server_id, sle_uuid_t *uuid, uint16_t service_handle,
    uint16_t handle, errcode_t status)
{
    osal_printk("%s add property cbk callback server_id:%x, service_handle:%x,handle:%x, status:%x\r\n",
        SLE_SERVER_LOG, server_id, service_handle, handle, status);
    sle_uuid_print(uuid);
}
static void ssaps_add_descriptor_cbk(uint8_t server_id, sle_uuid_t *uuid, uint16_t service_handle,
    uint16_t property_handle, errcode_t status)
{
    osal_printk("%s add descriptor cbk callback server_id:%x, service_handle:%x, property_handle:%x, \
        status:%x\r\n", SLE_SERVER_LOG, server_id, service_handle, property_handle, status);
    sle_uuid_print(uuid);
}
static void ssaps_delete_all_service_cbk(uint8_t server_id, errcode_t status)
{
    osal_printk("%s delete all service callback server_id:%x, status:%x\r\n", SLE_SERVER_LOG,
        server_id, status);
}
static errcode_t sle_ssaps_register_cbks(ssaps_read_request_callback ssaps_read_callback, ssaps_write_request_callback
    ssaps_write_callback)
{
    errcode_t ret;
    ssaps_callbacks_t ssaps_cbk = {0};
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
        osal_printk("%s sle_ssaps_register_cbks,ssaps_register_callbacks fail :%x\r\n", SLE_SERVER_LOG,
            ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}

/* device通过handle向host发送数据：report */
static errcode_t sle_server_send_report_by_handle(ssaps_send_param_t *ssaps_send_param_str)
{
    if(ssaps_send_param_str == NULL ) {
        osal_printk("%s sle_server_send_report_by_handle fail, ssaps_send_param_str is NULL\r\n", SLE_SERVER_LOG);
        return ERRCODE_FAIL;
    }
    if(ssaps_send_param_str->data == NULL) {
        osal_printk("%s sle_server_send_report_by_handle fail, ssaps_send_param_str data is NULL\r\n", SLE_SERVER_LOG);
        return ERRCODE_FAIL;
    }
    ssaps_ntf_ind_t param = { 0 };
    param.handle = ssaps_send_param_str->handle;
    param.type = SSAP_PROPERTY_TYPE_VALUE;
    param.value = ssaps_send_param_str->data;
    param.value_len = ssaps_send_param_str->len;
    return ssaps_notify_indicate(g_server_id, ssaps_send_param_str->conn_id, &param);
}

/* 初始化server */
static errcode_t sle_server_reg(sle_uuid_t server_app_uuid)
{
    errcode_t ret = ERRCODE_SLE_SUCCESS;
    ret = ssaps_register_server(&server_app_uuid, &g_server_id);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle mouse server reg fail, ret:%x\r\n", ret);
    }
    return ret;
}

static void sle_server_add_and_start_service(uint8_t type, uint16_t *property_ntf_hdl)
{
    switch(type)
    {
        case HID_SERVICE:
            sle_server_add_and_start_hid_service(g_server_id, property_ntf_hdl);
            break;
        case DIS_SERVICE:
            sle_server_add_and_start_dis_service(g_server_id, property_ntf_hdl);
            break;
        case CHARGE_SERVICE:
            sle_server_add_and_start_charge_service(g_server_id, property_ntf_hdl);
            break;
        case USB_SERVICE:
            sle_server_add_and_start_usb_service(g_server_id, property_ntf_hdl);
            break;
        default:
            osal_printk("service type error\r\n");
            break;
    }
}

static errcode_t sle_start_server(void)
{
    osal_printk("%s enter sle_start_server!\r\n", SLE_SERVER_LOG);
    errcode_t ret = ERRCODE_SLE_SUCCESS;
    ssap_exchange_info_t ssaps_param = { 0 };
    ssaps_param.mtu_size = SERVER_OWN_MTU;
    ssaps_param.version = VERSION_1;
    ret |= ssaps_set_info(g_server_id, &ssaps_param);
    ret |= sle_server_adv_init();
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_start_server fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }
    osal_printk("%s sle_start_server ok!\r\n", SLE_SERVER_LOG);
    return ret;
}

static void sle_connect_state_changed_cbk(uint16_t conn_id, const sle_addr_t *addr,
    sle_acb_state_t conn_state, sle_pair_state_t pair_state, sle_disc_reason_t disc_reason)
{
    osal_printk("%s connect state changed conn_id:0x%02x, conn_state:0x%x, pair_state:0x%x, disc_reason:0x%x\r\n", SLE_SERVER_LOG, \
        conn_id, conn_state, pair_state, disc_reason);
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
        // sle_remove_paired_remote_device(addr);
        msg_node.msg_type = APP_MSG_TYPE_SLE;
        msg_node.sub_type = APP_SLE_DISCONNECT_SUCCESS;
        msg_node.length = sizeof(connect_paire_message_t);
        con_msg_t.conn_id = conn_id;
        (void)memcpy_s(&(con_msg_t.addr), sizeof(sle_addr_t), addr, sizeof(sle_addr_t)); 
        (void)memcpy_s(msg_node.data, sizeof(connect_paire_message_t), &con_msg_t, sizeof(connect_paire_message_t));
    	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
    }
}

static void sle_pair_complete_cbk(uint16_t conn_id, const sle_addr_t *addr, errcode_t status)
{
    osal_printk("%s pair complete conn_id:%02x, status:%x\r\n", SLE_SERVER_LOG,
        conn_id, status);
    osal_printk("%s pair complete addr:%02x:**:**:**:%02x:%02x\r\n", SLE_SERVER_LOG,
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
    sle_trans_sig_cap_req(conn_id);
}

static void sle_connect_param_update_cb(uint16_t conn_id, errcode_t status, const sle_connection_param_update_evt_t *param)
{
    osal_printk("%s interval %d*0.125ms latency %d supervision %d*10ms, status=%x\r\n", SLE_SERVER_LOG, param->interval, param->latency, param->supervision, status);
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

static void sle_phy_update_cb(uint16_t conn_id, errcode_t status, const sle_set_phy_t *param)
{
    osal_printk("%s sle_phy_update_cb status %x\r\n", SLE_SERVER_LOG, status);
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

#ifdef CONFIG_SAMPLE_SUPPORT_MOUSE_BODY
    if(rate == 2)
    {
        extern void mouse_sle_low_latency_switch_work_report_rate_done_timer_start(void);
        mouse_sle_low_latency_switch_work_report_rate_done_timer_start();
    }
#endif
    osal_printk("%s sle_low_latency_cb status %x rate %d\r\n", SLE_SERVER_LOG, status, rate);

    app_message_t msg_node = { 0 };
    low_latency_message_t low_latency_msg = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_LOW_LATENCY_SWITCH_REPORT_RATE;
    msg_node.length = sizeof(low_latency_message_t);
    low_latency_msg.status = status;
    low_latency_msg.rate = rate;
    (void)memcpy_s(msg_node.data, sizeof(low_latency_message_t), &low_latency_msg, sizeof(low_latency_message_t));
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_enable_cbk(uint8_t status)
{
    osal_printk("%s sle enable callback status:0x%02x\r\n", SLE_SERVER_LOG, status);
    if (status != ERRCODE_SUCC) {
        return;
    }
    app_message_t msg_node = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_ENABLE_COMPLETE;
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_disable_cbk(uint8_t status)
{
    osal_printk("%s sle disable callback status:0x%02x\r\n", SLE_SERVER_LOG, status);
    if (status != ERRCODE_SUCC) {
        return;
    }
    app_message_t msg_node = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_DISABLE_COMPLETE;
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static void sle_trans_sig_cap_req(uint16_t conn_id)
{
    osal_printk("transmission_signal_capability_req\n");
    sle_transmission_signal_capability_bit_t tm_sig_params = {0};
    tm_sig_params.trans_mode = 1;
    tm_sig_params.mtu = 1;
    tm_sig_params.mps = 1;
    tm_sig_params.version = 1;
    sle_transmission_signal_capability_req(conn_id, &tm_sig_params);
}

static errcode_t sle_config_own_addr(sle_addr_t *local_addr)
{
    if(local_addr == NULL) {
        return ERRCODE_FAIL;
    }
    osal_printk("%s enter sle_config_own_addr!\r\n", SLE_SERVER_LOG);
    sle_config_server_own_addr(local_addr);
    return ERRCODE_SUCC;
}

static errcode_t sle_config_adv_name(uint8_t *adv_name)
{
    if(adv_name == NULL) {
        return ERRCODE_FAIL;
    }
    osal_printk("%s enter sle_config_adv_name!, sle_name_config=%s\r\n", SLE_SERVER_LOG, adv_name);
    sle_config_server_adv_name(adv_name);
    return ERRCODE_SUCC;
}

static errcode_t sle_set_adv_default_announce_param(void)
{
    sle_set_default_announce_param();
    return ERRCODE_SUCC;
}

static errcode_t sle_set_adv_default_announce_data(void)
{
    sle_set_default_announce_data();
    return ERRCODE_SUCC;
}

static void sle_config_announce_param(announce_param_t *announce_adv_param)
{
    if(announce_adv_param == NULL){
        osal_printk("%s sle_config_announce_param is NULL!\r\n", SLE_SERVER_LOG);
        return;
    }
    osal_printk("%s enter sle_config_announce_param!\r\n", SLE_SERVER_LOG);
    sle_config_announce_adv_param(announce_adv_param);
}

static errcode_t sle_stop_announce_adv(void)
{
    osal_printk("%s enter sle_stop_announce_adv!\r\n", SLE_SERVER_LOG);
    return sle_stop_announce(SLE_ADV_HANDLE_DEFAULT);
}

static errcode_t sle_start_announce_adv(void)
{
    return sle_start_announce(SLE_ADV_HANDLE_DEFAULT);
}

static errcode_t sle_server_start_update_interval(interval_param_t *interval_param_str)
{
    if(interval_param_str == NULL) {
        osal_printk("%s sle_server_start_update_interval fail, interval_param_str is NULL\r\n", SLE_SERVER_LOG);
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
    osal_printk("%s sle_server_start_update_interval, ret=%x\r\n", SLE_SERVER_LOG, ret);
    return ret;
}

static errcode_t sle_enable_sle(void)
{
    osal_printk("%s enter sle_enable_sle!\r\n", SLE_SERVER_LOG);
    return enable_sle();
}

static errcode_t sle_disable_sle(void)
{
    osal_printk("%s enter sle_disable_sle!\r\n", SLE_SERVER_LOG);
    return disable_sle();
}

static void sle_power_on_cbk(uint8_t status)
{
    osal_printk("%s sle power on status: 0x%02x\r\n", SLE_SERVER_LOG, status);
    if(status != ERRCODE_SLE_SUCCESS){
        return;
    }
    app_message_t msg_node = { 0 };
    msg_node.msg_type = APP_MSG_TYPE_SLE;
    msg_node.sub_type = APP_SLE_POWER_ON_COMPLETE;
	sle_write_msgqueue((uint8_t *)&msg_node, sizeof(app_message_t));
}

static errcode_t sle_server_start_disconnect(sle_addr_t *addr)
{
    errcode_t ret = ERRCODE_FAIL;
    if(addr == NULL) {
        return ret;
    }
    ret = sle_disconnect_remote_device(addr);
    osal_printk("%s start disconnect, ret=%x\r\n", SLE_SERVER_LOG, ret);
    return ret;
}

void sle_server_state_func_register(sle_state_handler_t *handler)
{
    handler->ssaps_send_data_by_handle = sle_server_send_report_by_handle;
    handler->sle_server_reg = sle_server_reg;
    handler->sle_server_add_and_start_service = sle_server_add_and_start_service;
    handler->sle_start_server = sle_start_server;
    handler->sle_config_announce_param = sle_config_announce_param;
    handler->sle_stop_announce = sle_stop_announce_adv;
    handler->sle_start_announce = sle_start_announce_adv;
    handler->sle_start_disconnect = sle_server_start_disconnect;
    handler->sle_start_interval_update = sle_server_start_update_interval;
    handler->sle_enable_sle = sle_enable_sle;
    handler->sle_disable_sle = sle_disable_sle;
    handler->sle_config_own_addr = sle_config_own_addr;
    handler->sle_config_adv_name = sle_config_adv_name;
    handler->sle_set_adv_default_announce_data = sle_set_adv_default_announce_data;
    handler->sle_set_adv_default_announce_param = sle_set_adv_default_announce_param;
}

static errcode_t sle_conn_register_cbks(void)
{
    errcode_t ret;
    sle_connection_callbacks_t conn_cbks = {0};
    conn_cbks.connect_state_changed_cb = sle_connect_state_changed_cbk;
    conn_cbks.pair_complete_cb = sle_pair_complete_cbk;
    conn_cbks.connect_param_update_cb = sle_connect_param_update_cb;
    conn_cbks.set_phy_cb = sle_phy_update_cb;
    conn_cbks.low_latency_cb = sle_low_latency_cb;
    ret = sle_connection_register_callbacks(&conn_cbks);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_conn_register_cbks,sle_connection_register_callbacks fail :%x\r\n",
        SLE_SERVER_LOG, ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}

static errcode_t sle_dev_register_cbks(void)
{
    errcode_t ret = 0;
    sle_dev_manager_callbacks_t dev_mgr_cbks = {0};
    dev_mgr_cbks.sle_power_on_cb = sle_power_on_cbk;
    dev_mgr_cbks.sle_enable_cb = sle_enable_cbk;
    dev_mgr_cbks.sle_disable_cb = sle_disable_cbk;
    ret = sle_dev_manager_register_callbacks(&dev_mgr_cbks);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_dev_register_cbks,register_callbacks fail :%x\r\n",
            SLE_SERVER_LOG, ret);
        return ret;
    }

    enable_sle();

    return ERRCODE_SLE_SUCCESS;
}

static errcode_t sle_announce_register_cbks(void)
{
    errcode_t ret;
    sle_announce_seek_callbacks_t seek_cbks = {0};
    seek_cbks.announce_enable_cb = sle_server_announce_enable_cbk;
    seek_cbks.announce_disable_cb = sle_server_announce_disable_cbk;
    seek_cbks.announce_terminal_cb = sle_server_announce_terminal_cbk;
    ret = sle_announce_seek_register_callbacks(&seek_cbks);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_announce_register_cbks,register_callbacks fail :%x\r\n",
        SLE_SERVER_LOG, ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}

/* 初始化server */
errcode_t sle_server_init(ssaps_read_request_callback ssaps_read_callback, 
                                ssaps_write_request_callback ssaps_write_callback)
{
    errcode_t ret;
    
    ret = sle_dev_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_server_init,sle_dev_register_cbks fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    ret = sle_conn_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_server_init,sle_conn_register_cbks fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }
    ret = sle_ssaps_register_cbks(ssaps_read_callback, ssaps_write_callback);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_server_init,sle_ssaps_register_cbks fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }
    ret = sle_announce_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_server_init,sle_announce_register_cbks fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    osal_printk("%s sle_server_init ok\r\n", SLE_SERVER_LOG);
    return ERRCODE_SLE_SUCCESS;
}

