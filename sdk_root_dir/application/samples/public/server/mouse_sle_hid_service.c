/**
 * Copyright (c) Triductor. 2023-2023. All rights reserved.
 * Description: Sle Mouse HID Server source.
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
#include "mouse_sle_hid_service.h"
#include "osal_debug.h"
#include "mouse_sle_service_cfg.h"

static uint8_t g_hid_server_id = 0;

static uint8_t g_cccd[]  =  {0x01, 0x0};

/*hid service data*/
#define HID_ELEMENT_NUM            6
static uint16_t g_sle_hid_group_hdl[HID_ELEMENT_NUM] = {0};
/*hid service*/
static const uint8_t g_sle_hid_group_uuid[HID_ELEMENT_NUM][SLE_UUID_LEN] = {
    /* Human Interface Device service UUID. */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x06 },
    /* Report characteristic UUID. 输入报告信息 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x10 },
    /* CCCD */
    { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
      0x00, 0x10, 0x00, 0x00, 0x02, 0x29, 0x00, 0x00 },
    /* Report Reference characteristic UUID. 报告索引信息 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x10 },
    /* Report Map characteristic UUID. 类型和格式描述 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x39, 0x10 },
    /* Hid Control Point characteristic UUID.  工作状态指示 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x10 },
};

static uint32_t g_hid_service_property_operate[HID_ELEMENT_NUM] = {
    0,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_NOTIFY,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE | SSAP_OPERATE_INDICATION_BIT_DESCRITOR_WRITE | SSAP_OPERATE_INDICATION_BIT_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_NOTIFY,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE
};

#define SLE_HID_REPORT_LENGTH    4
#define SLE_SRV_ENCODED_REPORT_LEN 8
static uint8_t g_input_report_descriptor[SLE_SRV_ENCODED_REPORT_LEN] = {0};
static uint8_t g_sle_hid_input_report[SLE_HID_REPORT_LENGTH] = {0};
static uint8_t g_sle_hid_control_point = 0x01;
static const uint8_t g_sle_hid_report_map[] = {
    0x00,                       /* type indicate */
    0x05, 0x01,                 /* Usage Page (Generic Desktop)             */
    0x09, 0x02,                 /* Usage (Mouse)                            */
    0xA1, 0x01,                 /* Collection (Application)                 */
    0x09, 0x01,                 /*  Usage (Pointer)                         */
    0xA1, 0x00,                 /*  Collection (Physical)                   */
    0x85, SLE_HID_REPORT_ID,    /*   Report ID  */
    0x05, 0x09,                 /*      Usage Page (Buttons)                */
    0x19, 0x01,                 /*      Usage Minimum (01)                  */
    0x29, 0x05,                 /*      Usage Maximum (05)                  */
    0x15, 0x00,                 /*      Logical Minimum (0)                 */
    0x25, 0x01,                 /*      Logical Maximum (1)                 */
    0x95, 0x05,                 /*      Report Count (5)                    */
    0x75, 0x01,                 /*      Report Size (1)                     */
    0x81, 0x02,                 /*      Input (Data, Variable, Absolute)    */
    0x95, 0x01,                 /*      Report Count (1)                    */
    0x75, 0x03,                 /*      Report Size (3)                     */
    0x81, 0x01,                 /*      Input (Constant)    ;3 bit padding  */
    0x05, 0x01,                 /*      Usage Page (Generic Desktop)        */
    0x09, 0x30,                 /*      Usage (X)                           */
    0x09, 0x31,                 /*      Usage (Y)                           */
    0x16, 0x00, 0x80,     /*Logical Minimum (-32768)*/
    0x26, 0xff, 0x7f,     /*Logical Maximum (32767)*/
    0x75, 0x10, /* Report Size (16) */
    0x95, 0x02,                 /*      Report Count (2)                    */
    0x81, 0x06,                 /*      Input (Data, Variable, Relative)    */
    0x05, 0x01,                 /*      Usage Page (Generic Desktop)        */
    0x09, 0x38,                 /*      Usage (Wheel)                       */
    0x15, 0x81,                 /*      Logical Minimum (-127)              */
    0x25, 0x7F,                 /*      Logical Maximum (127)               */
    0x75, 0x08,                 /*      Report Size (8)                     */
    0x95, 0x01,                 /*      Report Count (1)                    */
    0x81, 0x06,                 /*      Input (Data, Variable, Relative)    */
    0xC0,                       /* End Collection,End Collection            */
    0xC0,                       /* End Collection,End Collection            */
};

static errcode_t sle_hid_property_and_descriptor_add(void)
{
    errcode_t ret = ERRCODE_SLE_SUCCESS;
    uint16_t service_hdl = g_sle_hid_group_hdl[SLE_SERVICE_INDEX0];
    ret = sle_service_add_property_interface(g_hid_server_id, service_hdl, g_sle_hid_group_uuid[SLE_SERVICE_INDEX1], g_hid_service_property_operate[SLE_SERVICE_INDEX1],
                                            SLE_HID_REPORT_LENGTH, g_sle_hid_input_report, &g_sle_hid_group_hdl[SLE_SERVICE_INDEX1]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle hid add report fail, ret:%x, indet:%x\r\n", ret, SLE_SERVICE_INDEX1);
        return ERRCODE_SLE_FAIL;
    }
    osal_printk("[uuid server] sle hid add report, proterty hdl:%x\r\n", g_sle_hid_group_hdl[SLE_SERVICE_INDEX1]);

    ret = sle_service_add_descriptor_interface(g_hid_server_id, service_hdl, g_hid_service_property_operate[SLE_SERVICE_INDEX2], sizeof(g_cccd), g_cccd, g_sle_hid_group_hdl[SLE_SERVICE_INDEX1]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle hid add cccd fail, ret:%x, indet:%x\r\n", ret, SLE_SERVICE_INDEX2);
        return ERRCODE_SLE_FAIL;
    }

    g_input_report_descriptor[0] = SLE_HID_REPORT_ID;   // [1] : report id
    g_input_report_descriptor[1] = 0x1;   // [1] : input
    g_input_report_descriptor[2] = g_sle_hid_group_hdl[SLE_SERVICE_INDEX1]; // [2] rpt handle low
    g_input_report_descriptor[3] = 0;     // [3] rpt handle high
    ret = sle_service_add_property_interface(g_hid_server_id, service_hdl, g_sle_hid_group_uuid[SLE_SERVICE_INDEX3],
        g_hid_service_property_operate[SLE_SERVICE_INDEX3], SLE_SRV_ENCODED_REPORT_LEN, g_input_report_descriptor, &g_sle_hid_group_hdl[SLE_SERVICE_INDEX3]);
    
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle hid add report ref fail, ret:%x, indet:%x\r\n", ret, SLE_SERVICE_INDEX3);
        return ERRCODE_SLE_FAIL;
    }
    ret = sle_service_add_property_interface(g_hid_server_id, service_hdl, g_sle_hid_group_uuid[SLE_SERVICE_INDEX4],
        g_hid_service_property_operate[SLE_SERVICE_INDEX4], sizeof(g_sle_hid_report_map), (uint8_t *)g_sle_hid_report_map, &g_sle_hid_group_hdl[SLE_SERVICE_INDEX4]);

    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle hid add report map ref fail, ret:%x, indet:%x\r\n", ret,
            SLE_SERVICE_INDEX4);
        return ERRCODE_SLE_FAIL;
    }
    ret = sle_service_add_property_interface(g_hid_server_id, service_hdl, g_sle_hid_group_uuid[SLE_SERVICE_INDEX5],
        g_hid_service_property_operate[SLE_SERVICE_INDEX5], sizeof(uint8_t), &g_sle_hid_control_point, &g_sle_hid_group_hdl[SLE_SERVICE_INDEX5]);

    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle hid add hid ctrl point fail, ret:%x, indet:%x\r\n", ret,
            SLE_SERVICE_INDEX5);
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

static errcode_t sle_hid_service_add(void)
{
    errcode_t ret;
    sle_uuid_t service_uuid = {0};
    ret = sle_set_uuid(g_sle_hid_group_uuid[SLE_SERVICE_INDEX0], &service_uuid);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle hid uuid set fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }

    ret = ssaps_add_service_sync(g_hid_server_id, &service_uuid, true, &g_sle_hid_group_hdl[SLE_SERVICE_INDEX0]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle hid add service fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_server_add_and_start_hid_service(uint8_t server_id, uint16_t* property_ntf_hdl)
{
    errcode_t ret;
    g_hid_server_id = server_id;
    if (sle_hid_service_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_hid_server_id);
        return ERRCODE_SLE_FAIL;
    }

    if (sle_hid_property_and_descriptor_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_hid_server_id);
        return ERRCODE_SLE_FAIL;
    }

    ret = ssaps_start_service(g_hid_server_id, g_sle_hid_group_hdl[SLE_SERVICE_INDEX0]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle hid start service fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }
    osal_printk("[uuid server] sle uuid add service out\r\n");
    *property_ntf_hdl = g_sle_hid_group_hdl[SLE_SERVICE_INDEX1];
    return ERRCODE_SLE_SUCCESS;
}