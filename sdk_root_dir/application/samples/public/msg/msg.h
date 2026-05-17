/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: SLE MEssage header \n
 * Author: Triductor \n
 * History: \n
 * 2023-04-03, Create file. \n
 */
#ifndef MESSAGE_H
#define MESSAGE_H
#include "sle_connection_manager.h"
#include "sle_ssap_client.h"
#include "sle_ssap_stru.h"

#ifdef CONFIG_SAMPLE_SUPPORT_MOUSE_BODY
#define SLE_MSG_QUEUE_LEN       8
#else 
#define SLE_MSG_QUEUE_LEN       16
#endif
#define SLE_MSG_QUEUE_DATA_MAX_SIZE  264
#define SLE_MSG_QUEUE_MAX_SIZE  (SLE_MSG_QUEUE_DATA_MAX_SIZE + 4)

#define SLE_ADDR_SIZE       6
#define SLE_CLIENT_ID       0
#define SLE_PROPERTY_MAXNUM       8

/* APP消息类型 */
typedef enum {
    APP_SLE_POWER_ON_COMPLETE = 0,  //0x00
    APP_SLE_ENABLE_COMPLETE ,   //0x01
    APP_SLE_DISABLE_COMPLETE ,   //0x02
    APP_SLE_CLIENT_SEEK_ENABLE, //0x03
    APP_SLE_CLIENT_SEEK_DISABLE,    //0x04
    APP_SLE_CLIENT_SEEK_COMPLETE,   //0x05
    APP_SLE_CLIENT_SEEK_DISABLE_ERROR,   //0x06
    APP_SLE_CONNECT_SUCCESS,    //0x07
    APP_SLE_DISCONNECT_SUCCESS, //0x08
    APP_SLE_PAIRE_COMPLETE,     //0x09
    APP_SLE_MTU_EXCHANGE_SUCCESS,   //0x0a
    APP_SLE_INTERVAL_UPDATE_SUCCESS,    //0x0b
    APP_SLE_UPDATE_PHY_SUCCESS,    //0x0c
    APP_SLE_FIND_STRUCT_SUCCESS,    //0x0d
    APP_SLE_FIND_PROPERTY_SUCCESS,    //0x0e
    APP_SLE_FIND_STRUCT_CMP_SUCCESS,    //0x0f
    APP_SLE_STOP_ANNOUNCE_SUCCESS,    //0x10
    APP_SLE_START_ANNOUNCE,    //0x11
    APP_SLE_STOP_ANNOUNCE,    //0x12
    APP_SLE_START_CONNECT, //0x13
    APP_SLE_START_DISCONNECT, //0x14
    APP_SLE_TRIGGER_MATCH_CODE,     //0x15
    APP_SLE_WRITE_REQUEST_SUCCESS,     //0x16
    APP_SLE_LOW_LATENCY_SWITCH_REPORT_RATE,     //0x17
} app_task_type_sle_t;

typedef enum {
    APP_MSG_TYPE_BLE = 0,
    APP_MSG_TYPE_SLE,
    APP_MSG_TYPE_USB,
} app_msg_type_t;
    
typedef enum {
    DISABLE_DUMPLICATES_FILTER = 0,
    ENABLE_DUMPLICATES_FILTER,
} seek_filter_type_t;

typedef enum {
    VERSION_0 = 0,
    VERSION_1,
} mtu_version_type_t;

/*message*/
typedef struct app_message{
    uint8_t   msg_type; 
    uint8_t   sub_type;
    uint16_t  length;
    uint8_t   data[SLE_MSG_QUEUE_DATA_MAX_SIZE];
} app_message_t;

typedef struct mtu_message{
    uint16_t conn_id; 
    uint32_t mtu_size;
    uint8_t  resv[2];
} mtu_message_t;

typedef struct connect_paire_message{
    uint16_t conn_id; 
    sle_addr_t addr;
} connect_paire_message_t;

typedef struct interval_update_message{
    uint16_t conn_id; 
    uint16_t inerval;
} interval_update_message_t;

typedef struct phy_update_message{
    uint16_t conn_id; 
    sle_set_phy_t sle_phy_param;
    uint8_t  resv[2];
} phy_update_message_t;

typedef struct low_latency_message{
    uint8_t status; 
    uint8_t rate;
    uint8_t  resv[2];
} low_latency_message_t;

typedef struct seek_result_message{
    int8_t rssi;
    uint8_t addr[SLE_ADDR_SIZE];
    uint8_t data_length;
    uint8_t data[255];
    uint8_t  resv[1];
} seek_result_message_t;

typedef struct service_result_message{
    uint16_t conn_id; 
    ssapc_find_service_result_t service_result;
    uint8_t resv;
} service_result_message_t;

typedef struct property_result_message{
    uint16_t conn_id; 
    uint16_t handle;
    sle_uuid_t uuid;
    uint8_t resv[3];
} property_result_message_t;

typedef struct structor_result_message{
    uint16_t conn_id; 
    ssapc_find_structure_result_t sle_structure_result;
} structor_result_message_t;

/*param*/
typedef struct property_uuid_param{
    uint16_t server_uuid;
    uint8_t property_num;
    uint16_t property_uuid[SLE_PROPERTY_MAXNUM];
    uint16_t property_handle[SLE_PROPERTY_MAXNUM];
    uint8_t resv[1];
} property_uuid_param_t;

typedef struct mtu_param{
    uint16_t conn_id; 
    uint32_t mtu;
    uint8_t resv[2];
} mtu_param_t;

typedef struct phy_param{
    uint16_t conn_id; 
    uint8_t format;
    uint8_t phy;
} phy_param_t;

typedef struct interval_param{
    uint16_t conn_id; 
    uint16_t interval_min;
    uint16_t interval_max;
    uint16_t max_latency;
    uint16_t supervision_timeout;
    uint8_t resv[2];
} interval_param_t;

typedef struct announce_param{
    uint32_t announce_interval_min;
    uint32_t announce_interval_max;
    uint16_t conn_interval_min;
    uint16_t conn_interval_max;
} announce_param_t;

typedef struct ssaps_send_param{
    uint16_t conn_id;
    uint16_t handle;
    uint8_t *data;
    uint16_t len;
    uint8_t resv[2];
} ssaps_send_param_t;

typedef struct ssapc_send_param{
    uint16_t conn_id;
    ssapc_write_param_t ssapc_write_param;
    uint8_t resv[1];
} ssapc_send_param_t;

void sle_create_msgqueue(void);
void sle_delete_msgqueue(void);
void sle_write_msgqueue(uint8_t *buffer_addr, uint16_t buffer_size);
int32_t sle_receive_msgqueue(uint8_t *buffer_addr, uint32_t *buffer_size);
void sle_rx_buf_init(uint8_t *buffer_addr, uint32_t *buffer_size);
#endif
