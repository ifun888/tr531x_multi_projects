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
#include "osal_debug.h"
#include "mouse_sle_service_cfg.h"

errcode_t sle_service_add_descriptor_interface(uint8_t server_id, uint16_t service_hdl, uint32_t property_operate, uint16_t len, uint8_t *data, uint16_t property_handle)
{
    if (data == NULL) {
        osal_printk("sle sample add descriptor interface param is NULL\r\n");
        return ERRCODE_SLE_FAIL;
    }
    ssaps_desc_info_t descriptor = {0};
    descriptor.permissions = SSAP_PERMISSION_READ | SSAP_PERMISSION_WRITE;
    descriptor.operate_indication = property_operate;
    descriptor.type = SSAP_DESCRIPTOR_CLIENT_CONFIGURATION;
    descriptor.value_len = len;
    descriptor.value = data;
    return ssaps_add_descriptor_sync(server_id, service_hdl, property_handle, &descriptor);
}

errcode_t sle_service_add_property_interface(uint8_t server_id, uint16_t service_hdl, const uint8_t *property_uuid, uint32_t property_operate, uint16_t len, uint8_t *data,
    uint16_t* property_hdl)
{
    if ((data == NULL) || (property_hdl == NULL)) {
        osal_printk("sle sample add property interface param is NULL\r\n");
        return ERRCODE_SLE_FAIL;
    }
    ssaps_property_info_t property = {0};
    errcode_t ret = sle_set_uuid(property_uuid, &property.uuid);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle charge uuid set fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }
    property.permissions = SSAP_PERMISSION_READ | SSAP_PERMISSION_WRITE;
    property.operate_indication = property_operate;
    property.value_len = len;
    property.value = data;
    return ssaps_add_property_sync(server_id, service_hdl, &property, property_hdl);
}
