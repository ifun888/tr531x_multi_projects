/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: SLE public header Source. \n
 * Author: Triductor \n
 * History: \n
 * 2023-07-17, Create file. \n
 */

#include "securec.h"
#include "sle_common.h"
#include "osal_debug.h"
#include "sle_errcode.h"
#include "cmsis_os2.h"
#include "common_def.h"
#include "osal_addr.h"
#include "server/server.h"
#include "server/server_adv.h"
#include "client/client.h"
#include "msg/msg.h"
#include "public_init.h"

static sle_init_trans_handler_t sle_init_trans_handler;

void uapi_sle_init_transfer_handler_register(sle_init_trans_handler_t *handler)
{
    sle_init_trans_handler.ssaps_read_callback = handler->ssaps_read_callback;
    sle_init_trans_handler.ssaps_write_callback = handler->ssaps_write_callback;
    sle_init_trans_handler.notification_cb = handler->notification_cb;
    sle_init_trans_handler.indication_cb = handler->indication_cb;
}

void uapi_sle_state_func_register(sle_type_t type, sle_state_handler_t *handler)
{
    if (type == TYPE_MAX) {
        osal_printk("uapi_sle_state_handler_register fail, input type error [%d]\r\n", type);
        return;
    }
    if(type == SLE_SERVER) {
        sle_server_state_func_register(handler);
    } else {
        sle_client_state_func_register(handler);
    }
}

sle_result_state_t sle_protocol_init(sle_type_t type)
{
    sle_result_state_t ret = SLE_STATE_SUCCESS;
    if (type == TYPE_MAX) {
        osal_printk("sle_protocol_init fail, input type error [%d]\r\n", type);
        return SLE_STATE_ERROR;
    }
    sle_create_msgqueue();
    if(type == SLE_SERVER) {
        errcode_t state = sle_server_init(sle_init_trans_handler.ssaps_read_callback, 
                                          sle_init_trans_handler.ssaps_write_callback);
        if (state != ERRCODE_SLE_SUCCESS) {
            ret = SLE_STATE_ERROR;
        }
    } else {
        sle_client_init(sle_init_trans_handler.notification_cb, 
                        sle_init_trans_handler.indication_cb);
    }
    return ret;
}

#define UUID_LEN_2 2
#define UUID_INDEX 14
#define UUID_16BIT_LEN 2
#define UUID_128BIT_LEN 16
static const uint8_t g_sle_multicon_base[] = { 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, \
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void encode2byte_little(uint8_t *_ptr, uint16_t data)
{
    *(uint8_t *)((_ptr) + 1) = (uint8_t)((data) >> 0x8);
    *(uint8_t *)(_ptr) = (uint8_t)(data);
}

void sle_uuid_set_base(sle_uuid_t *out)
{
    errcode_t ret;
    ret = memcpy_s(out->uuid, SLE_UUID_LEN, (uint8_t *)g_sle_multicon_base, SLE_UUID_LEN);
    if (ret != EOK) {
        osal_printk("sle_uuid_set_base memcpy fail\n");
        out->len = 0;
        return ;
    }
    out->len = UUID_LEN_2;
}

void sle_uuid_u2set(uint16_t u2, sle_uuid_t *out)
{
    sle_uuid_set_base(out);
    out->len = UUID_LEN_2;
    encode2byte_little(&out->uuid[UUID_INDEX], u2);
}

errcode_t sle_set_uuid(const uint8_t *uuid, sle_uuid_t *service_uuid)
{
    if (memcpy_s(service_uuid->uuid, SLE_UUID_LEN, uuid, SLE_UUID_LEN) != EOK) {
        osal_printk("sle usb_dongle usb_dongle set uuid fail\r\n");
        return ERRCODE_SLE_MEMCPY_FAIL;
    }
    service_uuid->len = SLE_UUID_LEN;
    return ERRCODE_SLE_SUCCESS;
}

void sle_uuid_print(sle_uuid_t *uuid)
{
    if (uuid == NULL) {
        osal_printk("%s uuid_print,uuid is null\r\n");
        return;
    }
    if (uuid->len == UUID_16BIT_LEN) {
        osal_printk("%s uuid: %02x %02x.\n", 
            uuid->uuid[14], uuid->uuid[15]); /* 14 15: uuid index */
    } else if (uuid->len == UUID_128BIT_LEN) {
        osal_printk("%s uuid: \n"); /* 14 15: uuid index */
        osal_printk("%s 0x%02x 0x%02x 0x%02x 0x%02x\n", uuid->uuid[0], uuid->uuid[1],
            uuid->uuid[2], uuid->uuid[3]);
        osal_printk("%s 0x%02x 0x%02x 0x%02x 0x%02x\n", uuid->uuid[4], uuid->uuid[5],
            uuid->uuid[6], uuid->uuid[7]);
        osal_printk("%s 0x%02x 0x%02x 0x%02x 0x%02x\n", uuid->uuid[8], uuid->uuid[9],
            uuid->uuid[10], uuid->uuid[11]);
        osal_printk("%s 0x%02x 0x%02x 0x%02x 0x%02x\n", uuid->uuid[12], uuid->uuid[13],
            uuid->uuid[14], uuid->uuid[15]);
    }
}