/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: SLE public header. \n
 * Author: Triductor \n
 * History: \n
 * 2023-07-17, Create file. \n
 */

#ifndef PUBLIC_INIT_H
#define PUBLIC_INIT_H

#include <stdint.h>
#include "errcode.h"
#include "sle_connection_manager.h"
#include "sle_device_discovery.h"
#include "server/server.h"
#include "msg/msg.h"
#include "sle_ssap_client.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

typedef enum sle_type
{
    SLE_SERVER,
    SLE_CLIENT,
    TYPE_MAX
} sle_type_t;

typedef enum sle_result_state
{
    SLE_STATE_SUCCESS,
    SLE_STATE_ERROR
} sle_result_state_t;

typedef enum {
    HID_SERVICE, 
    DIS_SERVICE, 
    CHARGE_SERVICE, 
    USB_SERVICE, 
} sle_service_type_t;

/*
* @brief  注册server
* @param  [in] server_app_uuid : 应用层app uuid。
* @return [out]              执行结果状态。
*/
typedef errcode_t (*sle_server_reg_func_t)(sle_uuid_t server_app_uuid);

/*
* @brief  添加并启动server侧的service
* @param  [in] service : 需要添加的service。
* @param  [out] ntf_handle : service属性的nofify handle。
* @return [out]              执行结果状态。
*/
typedef void (*sle_server_add_and_start_service_func_t)(uint8_t type, uint16_t *property_ntf_hdl);

/*
* @brief  启动server侧的服务：包括启动服务和开启广播。
* @param  [in] property_uuid : 属性uuid 结构体。
* @param  [out] property_uuid : 属性 handle。
* @return [out]              执行结果状态。
*/
typedef errcode_t (*sle_start_server_func_t)(void);

/*
* @brief  停止广播。
* @return [out]              执行结果状态。
*/
typedef errcode_t (*sle_stop_announce_func_t)(void);

/*
* @brief  开启广播。
* @return [out]              执行结果状态。
*/
typedef errcode_t (*sle_start_announce_func_t)(void);

/*
* @brief  使能sle。
* @return [out]              执行结果状态。
*/
typedef errcode_t (*sle_enable_sle_func_t)(void);

/*
* @brief  去使能sle。
* @return [out]              执行结果状态。
*/
typedef errcode_t (*sle_disable_sle_func_t)(void);

/*
* @brief  开启扫描。
* @param  [in] seek_interval : 扫描间隔，取值范围[0x0032, 0xFFFF]，time = N * 0.125ms。
* @return [out]                执行结果状态。
*/
typedef errcode_t (*sle_start_seek_func_t)(uint16_t seek_interval);

/*
* @brief  停止扫描。
* @param  无。
* @return [out]              执行结果状态。
*/
typedef errcode_t (*sle_stop_seek_func_t)(void);

/*
* @brief  发起连接。
* @param  [in] addr : 需要关联的设备地址。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_start_connect_func_t)(sle_addr_t *addr);

/*
* @brief  发起查找属性。
* @param  [in] conn_id : 连接id。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_start_find_struct_func_t)(uint16_t conn_id);

/*
* @brief  发起断链。
* @param  [in] addr : 需要断联的设备地址。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_start_disconnect_func_t)(sle_addr_t *addr);

/*
* @brief  发起配对。
* @param  [in] addr : 需要配对的设备地址。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_start_paire_func_t)(sle_addr_t *addr);

/*
* @brief  删除配对。
* @param  [in] addr : 需要删除配对的设备地址。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_remove_paire_func_t)(sle_addr_t *addr);

/*
* @brief  发起mtu交换请求。
* @param  [in] mtu_str : mtu结构体。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_start_mtu_change_func_t)(mtu_param_t *mtu_str);

/*
* @brief  发起物理层带宽更新请求。
* @param  [in] phy_param_str : phy_param结构体。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_start_phy_update_func_t)(phy_param_t *phy_param_str);

/*
* @brief  发起链路interval更新请求。
* @param  [in] interval_param_str : interval_param结构体
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_start_interval_update_func_t)(interval_param_t *interval_param_str);

/*
* @brief  配置广播参数。
* @param  [in] announce_adv_param : announce_adv_param结构体。
* @return [out]        执行结果状态。
*/
typedef void (*sle_config_announce_param_func_t)(announce_param_t *announce_adv_param);

/*
* @brief  配置本端addr地址。
* @param  [in] addr : addr地址。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_config_own_addr_func_t)(sle_addr_t *addr);

/*
* @brief  配置adv名称。
* @param  [in] name : 广播名称。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_config_adv_name_func_t)(uint8_t *name);

/*
* @brief  配置adv广播默认参数内容。
* @param  [in] 无。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_set_adv_default_announce_param_func_t)(void);

/*
* @brief  配置adv广播默认数据内容。
* @param  [in] 无。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_set_adv_default_announce_data_func_t)(void);

/*
* @brief  server根据handle发送数据。
* @param  [ssaps_send_param_str] ssaps_send_param结构体。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_ssaps_send_data_by_handle)(ssaps_send_param_t *ssaps_send_param_str);

/*
* @brief  client向server发送数据。
* @param  [ssapc_send_param_str] ssapc_send_param结构体。
* @return [out]        执行结果状态。
*/
typedef errcode_t (*sle_ssapc_send_data)(ssapc_send_param_t *ssapc_send_param_str);

typedef struct sle_init_trans_handler {
    ssaps_read_request_callback ssaps_read_callback;
    ssaps_write_request_callback ssaps_write_callback;
    ssapc_notification_callback notification_cb;
    ssapc_indication_callback indication_cb;
} sle_init_trans_handler_t;

typedef struct sle_state_handler {
    sle_ssaps_send_data_by_handle ssaps_send_data_by_handle;                        /*server端向client端发送数据接口*/
    sle_ssapc_send_data ssapc_send_data;                                            /*client端向server端发送数据接口*/
    sle_config_announce_param_func_t sle_config_announce_param;                     /*server端配置广播参数接口*/
    sle_config_own_addr_func_t sle_config_own_addr;                                 /*server端和client端配置自己addr接口*/
    sle_config_adv_name_func_t sle_config_adv_name;                                 /*server端配置广播名称接口*/
    sle_set_adv_default_announce_param_func_t sle_set_adv_default_announce_param;     /*server端配置广播默认参数接口*/
    sle_set_adv_default_announce_data_func_t sle_set_adv_default_announce_data;     /*server端配置广播默认内容接口*/
    sle_enable_sle_func_t sle_enable_sle;                                           /*server端和client端使能sle接口*/
    sle_disable_sle_func_t sle_disable_sle;                                         /*server端和client端去使能sle接口*/
    sle_server_reg_func_t sle_server_reg;                                           /*server注册接口*/
    sle_server_add_and_start_service_func_t sle_server_add_and_start_service;       /*server端添加并启动service接口*/
    sle_start_server_func_t sle_start_server;                                       /*server端发起广播和设置初始MTU接口*/
    sle_stop_announce_func_t sle_stop_announce;                                     /*server端停止广播接口*/
    sle_start_announce_func_t sle_start_announce;                                   /*server端启动广播接口*/
    sle_start_seek_func_t sle_start_seek;                                           /*client端发起扫描接口*/
    sle_stop_seek_func_t sle_stop_seek;                                             /*client端停止扫描接口*/
    sle_start_connect_func_t sle_start_connect;                                     /*client端发起连接接口*/
    sle_start_disconnect_func_t sle_start_disconnect;                               /*client端和server端发起去关联接口*/
    sle_start_paire_func_t sle_start_paire;                                         /*client端和server端发起配对接口*/
    sle_remove_paire_func_t sle_remove_paire;                                       /*client端和server端删除配对接口*/
    sle_start_find_struct_func_t sle_start_find_struct;                             /*client端发起查找属性接口*/
    sle_start_mtu_change_func_t sle_start_mtu_exchange;                             /*client端和server端发起mtu交换接口*/
    sle_start_phy_update_func_t sle_start_phy_update;                               /*client端和server端发起物理层带宽更新接口*/
    sle_start_interval_update_func_t sle_start_interval_update;                     /*client端和server端发起连接间隔更新接口*/
} sle_state_handler_t;

sle_result_state_t sle_protocol_init(sle_type_t type);
void uapi_sle_init_transfer_handler_register(sle_init_trans_handler_t *handler);
void uapi_sle_state_func_register(sle_type_t type, sle_state_handler_t *handler);
void sle_server_state_func_register(sle_state_handler_t *handler);
void sle_client_state_func_register(sle_state_handler_t *handler);

void sle_uuid_u2set(uint16_t u2, sle_uuid_t *out);
errcode_t sle_set_uuid(const uint8_t *uuid, sle_uuid_t *service_uuid);
void sle_uuid_print(sle_uuid_t *uuid);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif
