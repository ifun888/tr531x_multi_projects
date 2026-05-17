/**
 * Copyright (c) Triductor. 2023-2023. All rights reserved.
 * Description: Sle Mouse DIS Server source.
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
#include "../../products/mouse/mouse_cfg.h"
#include "osal_debug.h"
#include "mouse_sle_service_cfg.h"

static uint8_t g_dis_server_id = 0;

/*dis service data*/
#define DIS_ELEMENT_NUM 9
static uint16_t g_sle_dis_group_hdl[DIS_ELEMENT_NUM] = {0};
/*dis service*/
static const uint8_t g_sle_dis_group_uuid[DIS_ELEMENT_NUM][SLE_UUID_LEN] = {
    /* DIS service UUID. 设备信息管理 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x09, 0x06 },
    /* Device name characteristic UUID 设备名称 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x10},
    /* Device appearance characteristic 设备外观 */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x41, 0x10},
      /* Pnp Id characteristic UUID（设备序列号） */
    { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x10 },

    {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x2F, 0x10},
    
    {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x30, 0x10},

    {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x31, 0x10},

    {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x32, 0x10},

    {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
      0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x33, 0x10},
};

static uint32_t g_dis_service_property_operate[DIS_ELEMENT_NUM] = {
    0,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP | SSAP_OPERATE_INDICATION_BIT_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP | SSAP_OPERATE_INDICATION_BIT_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP | SSAP_OPERATE_INDICATION_BIT_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP | SSAP_OPERATE_INDICATION_BIT_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP | SSAP_OPERATE_INDICATION_BIT_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP | SSAP_OPERATE_INDICATION_BIT_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP | SSAP_OPERATE_INDICATION_BIT_WRITE,
    SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP | SSAP_OPERATE_INDICATION_BIT_WRITE,
};

#define MANUFACTURE_INFO (uint8_t *)"Triductor"
#define MANUFACTURE_PNP_TYPE (uint8_t *)"445566"
#define MANUFACTURE_PNP_ID (uint8_t *)"Triductor-tr531x"
#define HARDWARE_VER (uint8_t *)"112233"
#define FIRMWARE_VER (uint8_t *)"112233"
#define SOFTWARE_VER (uint8_t *)"112233"

#define MANUFACTURE_INFO_LENGTH sizeof(MANUFACTURE_INFO)-1
#define MANUFACTURE_PNP_TYPE_LENGTH sizeof(MANUFACTURE_PNP_TYPE)-1
#define MANUFACTURE_PNP_ID_LENGTH sizeof(MANUFACTURE_PNP_ID)-1
#define HARDWARE_VER_LENGTH sizeof(HARDWARE_VER)-1
#define FIRMWARE_VER_LENGTH sizeof(FIRMWARE_VER)-1
#define SOFTWARE_VER_LENGTH sizeof(SOFTWARE_VER)-1

#define APPEARANCE_LENGTH 3
static uint8_t g_dis_appearance[APPEARANCE_LENGTH] = {0x02, 0x05, 0x00};

static errcode_t sle_dis_property_and_descriptor_add(void)
{
    errcode_t ret = ERRCODE_SLE_SUCCESS;
    uint16_t service_hdl = g_sle_dis_group_hdl[SLE_SERVICE_INDEX0];
    ret |= sle_service_add_property_interface(g_dis_server_id, service_hdl, g_sle_dis_group_uuid[SLE_SERVICE_INDEX1], g_dis_service_property_operate[SLE_SERVICE_INDEX1],
                                            MOUSE_SLE_DEVICE_NAME_LEN, (uint8_t *)MOUSE_SLE_DEVICE_NAME, &g_sle_dis_group_hdl[SLE_SERVICE_INDEX1]);
    ret |= sle_service_add_property_interface(g_dis_server_id, service_hdl, g_sle_dis_group_uuid[SLE_SERVICE_INDEX2], g_dis_service_property_operate[SLE_SERVICE_INDEX2],
                                            APPEARANCE_LENGTH, g_dis_appearance, &g_sle_dis_group_hdl[SLE_SERVICE_INDEX2]);
    ret |= sle_service_add_property_interface(g_dis_server_id, service_hdl, g_sle_dis_group_uuid[SLE_SERVICE_INDEX3], g_dis_service_property_operate[SLE_SERVICE_INDEX3],
                                            MANUFACTURE_INFO_LENGTH, MANUFACTURE_INFO, &g_sle_dis_group_hdl[SLE_SERVICE_INDEX3]);
    ret |= sle_service_add_property_interface(g_dis_server_id, service_hdl, g_sle_dis_group_uuid[SLE_SERVICE_INDEX4], g_dis_service_property_operate[SLE_SERVICE_INDEX4],
                                            MANUFACTURE_PNP_TYPE_LENGTH, MANUFACTURE_PNP_TYPE, &g_sle_dis_group_hdl[SLE_SERVICE_INDEX4]);
    ret |= sle_service_add_property_interface(g_dis_server_id, service_hdl, g_sle_dis_group_uuid[SLE_SERVICE_INDEX5], g_dis_service_property_operate[SLE_SERVICE_INDEX5],
                                            MANUFACTURE_PNP_ID_LENGTH, MANUFACTURE_PNP_ID, &g_sle_dis_group_hdl[SLE_SERVICE_INDEX5]);
    ret |= sle_service_add_property_interface(g_dis_server_id, service_hdl, g_sle_dis_group_uuid[SLE_SERVICE_INDEX6], g_dis_service_property_operate[SLE_SERVICE_INDEX6],
                                            HARDWARE_VER_LENGTH, HARDWARE_VER, &g_sle_dis_group_hdl[SLE_SERVICE_INDEX6]);
    ret |= sle_service_add_property_interface(g_dis_server_id, service_hdl, g_sle_dis_group_uuid[SLE_SERVICE_INDEX7], g_dis_service_property_operate[SLE_SERVICE_INDEX7],
                                            FIRMWARE_VER_LENGTH, FIRMWARE_VER, &g_sle_dis_group_hdl[SLE_SERVICE_INDEX7]);
    ret |= sle_service_add_property_interface(g_dis_server_id, service_hdl, g_sle_dis_group_uuid[SLE_SERVICE_INDEX8], g_dis_service_property_operate[SLE_SERVICE_INDEX8],
                                            SOFTWARE_VER_LENGTH, SOFTWARE_VER, &g_sle_dis_group_hdl[SLE_SERVICE_INDEX8]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle dis add property fail, ret:%x, \r\n", ret);
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

static errcode_t sle_dis_service_add(void)
{
    errcode_t ret;
    sle_uuid_t service_uuid = {0};
    ret = sle_set_uuid(g_sle_dis_group_uuid[SLE_SERVICE_INDEX0], &service_uuid);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle dis uuid set fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }

    ret = ssaps_add_service_sync(g_dis_server_id, &service_uuid, true, &g_sle_dis_group_hdl[SLE_SERVICE_INDEX0]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle dis add service fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_server_add_and_start_dis_service(uint8_t server_id, uint16_t* property_ntf_hdl)
{
    errcode_t ret;
    g_dis_server_id = server_id;
    if (sle_dis_service_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_dis_server_id);
        return ERRCODE_SLE_FAIL;
    }

    if (sle_dis_property_and_descriptor_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_dis_server_id);
        return ERRCODE_SLE_FAIL;
    }

    ret = ssaps_start_service(g_dis_server_id, g_sle_dis_group_hdl[SLE_SERVICE_INDEX0]);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[uuid server] sle dis start service fail, ret:%x\r\n", ret);
        return ERRCODE_SLE_FAIL;
    }
    osal_printk("[uuid server] sle uuid add service out\r\n");
    *property_ntf_hdl = g_sle_dis_group_hdl[SLE_SERVICE_INDEX1];

    return ERRCODE_SLE_SUCCESS;
}