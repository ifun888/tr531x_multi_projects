/*
 * Copyright (c) Triductor. 2021-2023. All rights reserved.
 * Description: transmit
 * This file should be changed only infrequently and with great care.
 */
#include "transmit.h"
#include "diag.h"
#include "diag_adapt_layer.h"
#include "transmit_src.h"
#include "transmit_dst.h"
#include "transmit_cmd_id.h"
#include "transmit_cmd_ls.h"
#include "transmit_cmd_delete_file.h"
#include "transmit_send_recv_pkt.h"
#include "transmit_host.h"
#include "transmit_debug.h"
#include "include/transmit_msg.h"
#include "diag_service.h"

#define TRANSMIT_MSG_PROC_MAX 10

typedef struct {
    uint8_t cmd_id;
    transmit_pkt_recv_hook handler;
} transmit_cmd_ind_item_t;

#if (defined(CONFIG_DFX_SUPPORT_TRANSMIT_FILE) && (CONFIG_DFX_SUPPORT_TRANSMIT_FILE == DFX_YES))
STATIC transmit_cmd_ind_item_t g_transmit_cmd_id_tbl[] = {
    { DIAG_CMD_ID_TRANSMIT_START,      transmit_receiver_start},
    { DIAG_CMD_ID_TRANSMIT_NEGOTIATE,  transmit_receiver_negotiate },
    { DIAG_CMD_ID_TRANSMIT_REQUEST,    transmit_receiver_data_request },
    { DIAG_CMD_ID_TRANSMIT_REPLY,      transmit_receiver_data_reply },
    { DIAG_CMD_ID_TRANSMIT_NOTIFY,     transmit_receiver_notify },
    { DIAG_CMD_ID_TRANSMIT_STOP,       transmit_receiver_stop },
#if defined(CONFIG_DFX_SUPPORT_FILE_SYSTEM) && (CONFIG_DFX_SUPPORT_FILE_SYSTEM == DFX_YES)
    { DIAG_CMD_ID_TRANSMIT_LS,         transmit_cmd_ls},
    { DIAG_CMD_ID_TRANSMIT_DEL_FILE,   transmit_cmd_delete_file}
#endif
};

STATIC transmit_ctrl_t g_transmit_ctrl = {0};

STATIC errcode_t transmit_cmd_receiver(uint8_t cmd_id, void *cmd_param, uint16_t cmd_param_size, diag_option_t *option)
{
    uint32_t i;
    for (i = 0; i < sizeof(g_transmit_cmd_id_tbl) / sizeof(g_transmit_cmd_id_tbl[0]); i++) {
        transmit_cmd_ind_item_t *item = &g_transmit_cmd_id_tbl[i];
        if (item->cmd_id == cmd_id) {
            transmit_printf_receive_frame(cmd_id, cmd_param, cmd_param_size, option);
            item->handler(cmd_id, cmd_param, cmd_param_size, option);
            return ERRCODE_SUCC;
        }
    }
    return ERRCODE_NOT_SUPPORT;
}

STATIC errcode_t transmit_service_process(diag_ser_data_t *data)
{
    diag_ser_frame_t *req = (diag_ser_frame_t *)((uint8_t *)data + sizeof(diag_ser_data_t));
    uint8_t *usr_data = (uint8_t *)((uint8_t *)req + sizeof(diag_ser_frame_t));
    uint16_t size = data->header.length - (uint16_t)sizeof(diag_ser_frame_t);

    diag_option_t option = DIAG_OPTION_INIT_VAL;
    option.peer_addr = data->header.src;
    return transmit_cmd_receiver(req->cmd_id, usr_data, size, &option);
}

#if (defined(CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE) && (CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE == DFX_YES))
STATIC errcode_t transmit_host_start(transmit_type_t transmit_type, uint16_t channel_id, transmit_cfg_info_t *cfg_info,
    transmit_callback_t *callback)
{
    uint8_t msg_data[DFX_MSG_MAX_SIZE] = {0};
    transmit_msg_t *msg = (transmit_msg_t *)msg_data;
    errcode_t ret;
    char *host_file_name = NULL;
    char *device_file_name = NULL;

    transmit_host_start_param_t *start_param = dfx_malloc(0, sizeof(transmit_host_start_param_t));
    if (start_param == NULL) {
        return ERRCODE_MALLOC;
    }

    start_param->transmit_type = transmit_type;
    start_param->channel_id = channel_id;
    start_param->cfg_info = *cfg_info;
    start_param->callback = *callback;
    if (transmit_type == TRANSMIT_TYPE_FILE_UPSTREAM || transmit_type == TRANSMIT_TYPE_FILE_DOWNSTREAM) {
        uint32_t host_file_len = strlen(cfg_info->data.file_info.host_file_name);
        uint32_t dev_file_len = strlen(cfg_info->data.file_info.device_file_name);
        host_file_name = dfx_malloc(0, host_file_len + 1);
        device_file_name = dfx_malloc(0, dev_file_len + 1);
        if (host_file_name == NULL || device_file_name == NULL) {
            ret = ERRCODE_MALLOC;
            goto err;
        }

        (void)memset_s(host_file_name, host_file_len + 1, 0, host_file_len + 1);
        (void)memset_s(device_file_name, dev_file_len + 1, 0, dev_file_len + 1);
        (void)memcpy_s(host_file_name, host_file_len, cfg_info->data.file_info.host_file_name, host_file_len);
        (void)memcpy_s(device_file_name, dev_file_len, cfg_info->data.file_info.device_file_name, dev_file_len);
        start_param->cfg_info.data.file_info.host_file_name = host_file_name;
        start_param->cfg_info.data.file_info.device_file_name = device_file_name;
    }

    msg->msg_type = TRANSMIT_MSG_TYPE_HOST_START;
    msg->msg_len = sizeof(uint32_t);
    /* msg->msg_data 中存放start_param的地址 */
    *(uint32_t*)msg->msg_data = (uint32_t)(uintptr_t)start_param;

    ret = transmit_msg_write(DFX_MSG_ID_TRANSMIT_FILE, msg_data, sizeof(transmit_msg_t) + msg->msg_len, false);
    if (ret != ERRCODE_SUCC) {
        goto err;
    }
    return ERRCODE_SUCC;
err:
    if (host_file_name != NULL) {
        dfx_free(0, host_file_name);
    }
    if (device_file_name != NULL) {
        dfx_free(0, device_file_name);
    }
    dfx_free(0, start_param);
    return ret;
}
#endif

transmit_ctrl_t *transmit_get_ctrl(void)
{
    return &g_transmit_ctrl;
}

errcode_t uapi_transmit_init(void)
{
    errcode_t ret;
    if (g_transmit_ctrl.inited != 0) {
        return ERRCODE_SUCC;
    }

    (void)memset_s(&g_transmit_ctrl, sizeof(g_transmit_ctrl), 0, sizeof(g_transmit_ctrl));

    ret = transmit_timer_init();
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    g_transmit_ctrl.transmit_id = 0x10;

    ret = uapi_diag_service_register(DIAG_SER_FILE_TRANSMIT, transmit_service_process);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    g_transmit_ctrl.inited = 1;
    return ERRCODE_SUCC;
}

errcode_t uapi_transmit_deinit(void)
{
    if (g_transmit_ctrl.inited == 0) {
        return ERRCODE_SUCC;
    }

    (void)transmit_item_stop_all_items(false);

    if (g_transmit_ctrl.pkt_buf != NULL) {
        dfx_free(0, g_transmit_ctrl.pkt_buf);
        g_transmit_ctrl.pkt_size = 0;
    }

    (void)transmit_timer_deinit();
    (void)uapi_diag_service_register(DIAG_SER_FILE_TRANSMIT, NULL);
    (void)memset_s(&g_transmit_ctrl, sizeof(g_transmit_ctrl), 0, sizeof(g_transmit_ctrl));
    return ERRCODE_SUCC;
}

errcode_t uapi_transmit_host_start(transmit_type_t transmit_type, uint16_t channel_id, transmit_cfg_info_t *cfg_info,
    transmit_callback_t *callback)
{
#if (defined(CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE) && (CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE == DFX_YES))
    if (cfg_info == NULL || callback == NULL || transmit_type >= TRANSMIT_TYPE_MAX) {
        return ERRCODE_INVALID_PARAM;
    }

    if ((transmit_type == TRANSMIT_TYPE_FILE_UPSTREAM || transmit_type == TRANSMIT_TYPE_FILE_DOWNSTREAM) &&
        (cfg_info->data.file_info.host_file_name == NULL || cfg_info->data.file_info.device_file_name == NULL)) {
        return ERRCODE_INVALID_PARAM;
    }

    return transmit_host_start(transmit_type, channel_id, cfg_info, callback);
#else
    unused(transmit_type);
    unused(channel_id);
    unused(cfg_info);
    unused(callback);
    return ERRCODE_NOT_SUPPORT;
#endif
}

errcode_t uapi_transmit_host_stop(transmit_type_t transmit_type, uint16_t channel_id)
{
#if (defined(CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE) && (CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE == DFX_YES))
    uint8_t msg_data[DFX_MSG_MAX_SIZE] = {0};
    transmit_msg_t *msg = (transmit_msg_t *)msg_data;
    msg->msg_type = TRANSMIT_MSG_TYPE_HOST_STOP;
    msg->msg_len = sizeof(transmit_host_stop_param_t);
    transmit_host_stop_param_t *stop_param = (transmit_host_stop_param_t *)msg->msg_data;
    stop_param->transmit_type = transmit_type;
    stop_param->channel_id = channel_id;
    return transmit_msg_write(DFX_MSG_ID_TRANSMIT_FILE, msg_data, sizeof(transmit_msg_t) + msg->msg_len, false);
#else
    unused(transmit_type);
    unused(channel_id);
    return ERRCODE_NOT_SUPPORT;
#endif
}

errcode_t uapi_transmit_device_register_result_hook(transmit_callback_t *callback)
{
    if (callback == NULL) {
        return ERRCODE_INVALID_PARAM;
    }
    transmit_ctrl_t *transmit_ctrl = transmit_get_ctrl();
    transmit_ctrl->result_hook = (uintptr_t)callback->result_hook;
    transmit_ctrl->result_usr_data = callback->result_usr_data;
    return ERRCODE_SUCC;
}

errcode_t uapi_transmit_device_stop(void)
{
    transmit_msg_t msg;
    msg.msg_type = TRANSMIT_MSG_TYPE_DEVICE_STOP;
    msg.msg_len = 0;
    return transmit_msg_write(DFX_MSG_ID_TRANSMIT_FILE, (uint8_t *)(uintptr_t)&msg, sizeof(transmit_msg_t), false);
}

#endif /* (defined(CONFIG_DFX_SUPPORT_TRANSMIT_FILE) && (CONFIG_DFX_SUPPORT_TRANSMIT_FILE == DFX_YES)) */
