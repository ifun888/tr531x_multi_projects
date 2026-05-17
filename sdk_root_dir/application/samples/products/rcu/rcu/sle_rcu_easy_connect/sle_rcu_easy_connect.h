/**
 * Copyright (c) Triductor. 2023-2023. All rights reserved.
 *
 * Description: SLE Easy Connect Config. \n
 *
 * History: \n
 * 2023-09-21, Create file. \n
 */

#ifndef SLE_EASY_CONNECT_H
#define SLE_EASY_CONNECT_H
#include "sle_device_discovery.h"

#define SLE_MTU_SIZE_DEFAULT        300
#define UUID_16BIT_LEN 2
#define UUID_128BIT_LEN 16
#define DEVICE_IS_NEAR_RSSI (-33)
#define DEVICE_IS_NEAR_TIMES 3
#define SLE_EASY_CONNECT_SAMPLE_SEEK_INTERVAL 100
#define SLE_EASY_CONNECT_SAMPLE_SEEK_WINDOW 50
#define MAX_REPORT_DATA_LEN 20
#define SAMPLE_REPORT_ID 0xDD

#define MAX_RSII_SIZE 20
#define MAX_ADV_DATA_LEN  32
#define MAX_DEVICE_SIZE 10
#define TIME_THOUSANDS 1000
#define CONNECT_TIME_GAP 10
typedef struct {
    uint8_t uuid[4];
    uint8_t len;
    uint8_t type;
    uint8_t str[MAX_ADV_DATA_LEN];
} adv_vendor_s;

typedef struct {
    uint8_t addr[SLE_ADDR_LEN];
    uint8_t adv_len;
    uint8_t adv_data[MAX_ADV_DATA_LEN];
} scandata_s;

typedef struct {
    scandata_s buf;
    uint8_t status;  /* 0:no 1:ready */
    int8_t rssi_record[MAX_RSII_SIZE];
    uint32_t scan_pos;
    uint8_t type;
    long connect_time;
} sle_device_recode;

typedef struct {
    uint8_t  report_id;
    uint8_t  payload_len;
    uint16_t vendor_id;
    uint16_t product_type;
    uint8_t  mac_addr[SLE_ADDR_LEN];
    uint8_t  extra_data[MAX_REPORT_DATA_LEN];
} report_data_info;

void sle_sample_seek_enable_cbk(errcode_t status);
void sle_sample_seek_disable_cbk(errcode_t status);
void sle_sample_seek_result_info_cbk(sle_seek_result_info_t *seek_result_data);
void rcu_easy_connect_get_server_id(uint8_t server_id);
void rcu_easy_connect_get_conn_id(uint8_t conn_id);
void sle_easy_connect_start(void);
void sle_easy_connect_stop(void);

errcode_t sle_rcu_easy_connect_server_add(void);
errcode_t sle_start_scan(void);
errcode_t sle_rcu_send_cfg_info_by_handle(uint8_t *data, uint8_t len, uint16_t conn_id);
#endif