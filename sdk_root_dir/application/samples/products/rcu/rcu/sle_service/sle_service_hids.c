/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: SLE SERVICE HIDS \n
 *
 * History: \n
 * 2024-05-25, Create file. \n
 */
#include "securec.h"
#include "sle_errcode.h"
#include "sle_server.h"
#include "osal_addr.h"
#include "sle_service_hids.h"

#define HID_DEVICE_TYPE_NUM              1
#define HID_REF_OFFSET                   2
#define HID_SERVICE_STEP                 3

/* Hid Information characteristic not defined */
static uint8_t g_sle_hid_group_uuid[HID_ELEMENT_NUM][SLE_UUID_LEN] = {
    /* Human Interface Device service UUID. */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0B },
    /* Report characteristic UUID. 输入报告信息 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3C },
    /* CCCD */
    { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
      0x00, 0x10, 0x00, 0x00, 0x02, 0x29, 0x00, 0x00 },
    /* Report Reference characteristic UUID. 报告索引信息 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3B },
    /* Report Map characteristic UUID. 类型和格式描述 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x39 },
    /* Hid Control Point characteristic UUID.  工作状态指示 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3A },
};

static uint8_t g_hid_service_property[HID_ELEMENT_NUM] = {
    0,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_NOTIFY,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP | SSAP_OPERATE_INDICATION_BIT_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_NOTIFY,
    SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP,
};

static errcode_t sle_add_descriptor_interface(uint32_t properties, sle_service_hids_t* hid_service,
    uint16_t device_input)
{
    if (hid_service->cccd == NULL) {
        osal_printk("sle add descriptor interface param is NULL\r\n");
        return ERRCODE_SLE_FAIL;
    }
    ssaps_desc_info_t descriptor = {0};
    descriptor.permissions = SSAP_PERMISSION_READ | SSAP_PERMISSION_WRITE;
    descriptor.operate_indication = properties;
    descriptor.type = SSAP_DESCRIPTOR_CLIENT_CONFIGURATION;
    descriptor.value_len = hid_service->cccd_len;
    descriptor.value = hid_service->cccd;
    return ssaps_add_descriptor_sync(sle_get_server_id(),
        hid_service->item_handle[SLE_HID_INDEX_SERVICE].handle_out,
        hid_service->item_handle[device_input].handle_out, &descriptor);
}

static errcode_t sle_add_device_descriptor(sle_service_hids_t* hid_service, uint16_t index_multiple)
{
    errcode_t ret;
    uint16_t device_input = SLE_HID_INDEX_HID_CONTROL + 1 + HID_SERVICE_STEP * index_multiple;

    ret = sle_add_property(g_hid_service_property[HID_INDEX_REPORT], g_sle_hid_group_uuid[HID_INDEX_REPORT],
        SLE_INPUT_REPORT_LENGTH, hid_service->input_report, &hid_service->item_handle[device_input]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle add report fail, ret:%x, indet:%x\r\n",
                    ret, SLE_HID_INDEX_KEYBOARD_INPUT);
        return ERRCODE_SLE_FAIL;
    }

    ret = sle_add_descriptor_interface(g_hid_service_property[HID_INDEX_CCCD], hid_service, device_input);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle add cccd fail, ret:%x, indet:%x\r\n",
                    ret, SLE_HID_INDEX_KEYBOARD_CCCD);
        return ERRCODE_SLE_FAIL;
    }

    hid_service->input_report_descriptor[0] = 0x1;   // [1] : report id
    hid_service->input_report_descriptor[1] = 0x1;   // [1] : input
    // [2] rpt handle low
    hid_service->input_report_descriptor[2] = hid_service->item_handle[device_input].handle_out;
    hid_service->input_report_descriptor[3] = 0;     // [3] rpt handle high

    ret = sle_add_property(g_hid_service_property[HID_INDEX_REF], g_sle_hid_group_uuid[HID_INDEX_REF],
        SLE_SRV_ENCODED_REPORT_LEN, hid_service->input_report_descriptor,
        &hid_service->item_handle[device_input + HID_REF_OFFSET]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle add report ref fail, ret:%x, indet:%x\r\n",
                    ret, SLE_HID_INDEX_KEYBOARD_REF);
        return ERRCODE_SLE_FAIL;
    }
    return ret;
}

errcode_t sle_hids_property_and_descriptor_add(sle_service_hids_t* hid_service)
{
    errcode_t ret = ERRCODE_SLE_SUCCESS;
    // report_map_datas
    ret = sle_add_property(g_hid_service_property[HID_INDEX_MAP], g_sle_hid_group_uuid[HID_INDEX_MAP],
        hid_service->map_data_len, hid_service->report_map_datas, &hid_service->item_handle[SLE_HID_INDEX_REPORT_MAP]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle add report map ref fail, ret:%x, indet:%x\r\n",
                    ret, SLE_HID_INDEX_REPORT_MAP);
        return ERRCODE_SLE_FAIL;
    }

    // control_point
    ret = sle_add_property(g_hid_service_property[HID_INDEX_CONTROL], g_sle_hid_group_uuid[HID_INDEX_CONTROL],
        sizeof(hid_service->hid_control_point), &hid_service->hid_control_point,
        &hid_service->item_handle[SLE_HID_INDEX_HID_CONTROL]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle add hid ctrl point fail, ret:%x, indet:%x\r\n",
                    ret, SLE_HID_INDEX_HID_CONTROL);
        return ERRCODE_SLE_FAIL;
    }

    for (uint16_t i = 0; i < HID_DEVICE_TYPE_NUM; i++) {
        ret = sle_add_device_descriptor(hid_service, i);
    }

    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_rcu_hid_service_add(sle_service_hids_t* hid_service)
{
    errcode_t ret = ERRCODE_SLE_SUCCESS;
    
    // add HID service
    ret = sle_service_add(g_sle_hid_group_uuid[HID_INDEX_SERVICE],
        hid_service->item_handle, SLE_HID_INDEX_SERVICE, SLE_HID_INDEX_MAX);
    if (ret != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(sle_get_server_id());
        return ERRCODE_SLE_FAIL;
    }

    if (sle_hids_property_and_descriptor_add(hid_service) != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(sle_get_server_id());
        return ERRCODE_SLE_FAIL;
    }

    ret = ssaps_start_service(sle_get_server_id(), hid_service->item_handle[SLE_HID_INDEX_SERVICE].handle_out);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle rcu add HID service fail, ret:%x\r\n", SLE_RCU_SERVER_LOG, ret);
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}