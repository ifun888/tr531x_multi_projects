/**
 * Copyright (c) Triductor. 2023-2023. All rights reserved.
 * Description: Sle Mouse USB_DONGLE Server source.
 *
 * History:
 * 2023-09-01, Create file.
 */
#include "securec.h"
#include "errcode.h"
#include "osal_addr.h"
#include "sle_common.h"
#include "test_suite_uart.h"
#include "sle_errcode.h"
#include "sle_ssap_server.h"
#include "sle_connection_manager.h"
#include "sle_device_manager.h"
#include "sle_device_discovery.h"
#include "../public_init.h"
#include "osal_debug.h"
#include "mouse_sle_service_cfg.h"

static uint8_t g_usb_server_id = 0;

static uint8_t g_cccd[]  =  {0x01, 0x0};

#define USB_ELEMENT_NUM 3
static uint16_t g_sle_usb_group_hdl[USB_ELEMENT_NUM] = {0};

static uint8_t g_sle_usb_group_uuid[USB_ELEMENT_NUM][SLE_UUID_LEN] = {
    /* USB service UUID. USB 信息管理 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22 },
    /* USB Status property UUID. USB状态管理 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x23, 0x23},
    /* USB CCCD */
    { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
      0x00, 0x10, 0x00, 0x00, 0x02, 0x29, 0x00, 0x00 },
};

static uint32_t g_usb_service_property_operate[USB_ELEMENT_NUM] = {
    0,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP | SSAP_OPERATE_INDICATION_BIT_WRITE | SSAP_OPERATE_INDICATION_BIT_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE
};

static uint8_t g_usb_status = 0x00;

static errcode_t sle_usb_property_and_descriptor_add(void)
{
    errcode_t ret = ERRCODE_SLE_SUCCESS;
    uint16_t service_hdl = g_sle_usb_group_hdl[SLE_SERVICE_INDEX0];
    ret = sle_service_add_property_interface(g_usb_server_id, service_hdl, g_sle_usb_group_uuid[SLE_SERVICE_INDEX1], g_usb_service_property_operate[SLE_SERVICE_INDEX1],
                                            sizeof(uint8_t), &g_usb_status, &g_sle_usb_group_hdl[SLE_SERVICE_INDEX1]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle usb add report fail, ret:%x, indet:%x\r\n", ret, SLE_SERVICE_INDEX1);
        return ERRCODE_SLE_FAIL;
    }
    osal_printk("[uuid server] sle usb add report, proterty hdl:%x\r\n", g_sle_usb_group_hdl[SLE_SERVICE_INDEX1]);

    ret = sle_service_add_descriptor_interface(g_usb_server_id, service_hdl, g_usb_service_property_operate[SLE_SERVICE_INDEX2], sizeof(g_cccd), g_cccd, g_sle_usb_group_hdl[SLE_SERVICE_INDEX1]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle usb add cccd fail, ret:%x, indet:%x\r\n", ret, SLE_SERVICE_INDEX2);
        return ERRCODE_SLE_FAIL;
    }

    return ERRCODE_SLE_SUCCESS;
}

static errcode_t sle_usb_service_add(void)
{
    errcode_t ret;
    sle_uuid_t service_uuid = {0};
    ret = sle_set_uuid(g_sle_usb_group_uuid[SLE_SERVICE_INDEX0], &service_uuid);
    ret = ssaps_add_service_sync(g_usb_server_id, &service_uuid, true, &g_sle_usb_group_hdl[SLE_SERVICE_INDEX0]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle usb add service fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_server_add_and_start_usb_service(uint8_t server_id, uint16_t* property_ntf_hdl)
{
    errcode_t ret;
    g_usb_server_id = server_id;
    if (sle_usb_service_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_usb_server_id);
        return ERRCODE_SLE_FAIL;
    }

    if (sle_usb_property_and_descriptor_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_usb_server_id);
        return ERRCODE_SLE_FAIL;
    }

    ret = ssaps_start_service(g_usb_server_id, g_sle_usb_group_hdl[SLE_SERVICE_INDEX0]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle usb start service fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }
    osal_printk("[uuid server] sle uuid add service out\r\n");
    *property_ntf_hdl = g_sle_usb_group_hdl[SLE_SERVICE_INDEX1];

    return ERRCODE_SLE_SUCCESS;
}