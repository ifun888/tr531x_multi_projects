/*
 * Copyright (c) Triductor. 2021-2023. All rights reserved.
 * Description: transmit
 * This file should be changed only infrequently and with great care.
 */
#include "include/transmit_msg.h"
#include "transmit.h"
#include "transmit_src.h"
#include "transmit_dst.h"
#include "transmit_host.h"
#include "transmit_item.h"
#include "diag.h"
#include "diag_adapt_layer.h"
#include "diag_service.h"
#include "diag_msg.h"
#include "diag_dfx.h"

#define TRANSMIT_MSG_PROC_MAX 10

#if defined(CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK) && (CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK == DFX_YES)
STATIC transmit_msg_proc_t g_transmit_msg_proc[TRANSMIT_MSG_PROC_MAX] = { 0 };
#endif

#if (defined(CONFIG_DFX_SUPPORT_TRANSMIT_FILE) && (CONFIG_DFX_SUPPORT_TRANSMIT_FILE == DFX_YES))

STATIC void transmit_item_time_out(transmit_item_t *item, uint32_t cur_time)
{
    dfx_assert(item);
    if ((item->local_src) != 0) {
        transmit_src_item_process_timer(item, cur_time);
    } else {
        transmit_dst_item_process_timer(item, cur_time);
    }
}

STATIC void transmit_period_msg_proc(void)
{
    transmit_ctrl_t *transmit_ctrl = transmit_get_ctrl();
    transmit_item_t *item = NULL;
    uint32_t cur_sec = dfx_get_cur_second();
    for (int i = 0; i < CONFIG_DIAG_TRANSMIT_ITEM_CNT; i++) {
        item = &transmit_ctrl->item[i];
        if ((item->enable != 0) && (item->permanent == false) && (cur_sec > item->expiration)) {
            transmit_item_time_out(item, cur_sec);
        }
    }
    return;
}

STATIC errcode_t transmit_defult_msg_proc(const uint8_t *msg, uint32_t msg_len)
{
    unused(msg_len);

    transmit_msg_t *transmit_msg = (transmit_msg_t *)msg;

    switch (transmit_msg->msg_type) {
        case TRANSMIT_MSG_TYPE_TIMER:
            transmit_period_msg_proc();
            return ERRCODE_SUCC;
#if (defined(CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE) && (CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE == DFX_YES))
        case TRANSMIT_MSG_TYPE_HOST_START:
            {
                errcode_t ret;
                uint32_t param_addr = *(uint32_t *)transmit_msg->msg_data;
                transmit_host_start_param_t *start_param = (transmit_host_start_param_t *)(uintptr_t)param_addr;
                ret = transmit_host_send_start_cmd(start_param->transmit_type, start_param->channel_id,
                    &(start_param->cfg_info), &(start_param->callback));
                /* HOST_START消息中存放参数结构体的地址，使用后需释放参数结构体 */
                if (start_param->transmit_type == TRANSMIT_TYPE_FILE_UPSTREAM ||
                    start_param->transmit_type == TRANSMIT_TYPE_FILE_DOWNSTREAM) {
                    if (start_param->cfg_info.data.file_info.host_file_name != NULL) {
                        dfx_free(0, (void *)start_param->cfg_info.data.file_info.host_file_name);
                    }
                    if (start_param->cfg_info.data.file_info.device_file_name != NULL) {
                        dfx_free(0, (void *)start_param->cfg_info.data.file_info.device_file_name);
                    }
                }
                dfx_free(0, (void *)start_param);
                return ret;
            }
        case TRANSMIT_MSG_TYPE_HOST_STOP:
            {
                /* HOST_STOP消息中存放参数结构体的内容，无需释放 */
                transmit_host_stop_param_t *stop_param = (transmit_host_stop_param_t *)transmit_msg->msg_data;
                return transmit_host_send_stop_cmd(stop_param->transmit_type, (diag_addr)stop_param->channel_id);
            }
#endif
        case TRANSMIT_MSG_TYPE_DEVICE_STOP:
            return transmit_item_stop_all_items(true);
        default:
            break;
    }
    return ERRCODE_FAIL;
}

#if defined(CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK) && (CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK == DFX_YES)
STATIC errcode_t transmit_run_hook(uint32_t msg_id, const uint8_t *msg, uint32_t msg_len)
{
    transmit_msg_proc_t *msg_proc;
    for (int i = 0; i < TRANSMIT_MSG_PROC_MAX; i++) {
        msg_proc = &g_transmit_msg_proc[i];
        if ((msg_id > msg_proc->id_start) && (msg_id < msg_proc->id_end) &&
           (((transmit_msg_proc_hook)msg_proc->hook) != NULL)) {
            ((transmit_msg_proc_hook)msg_proc->hook)(msg_id, msg, msg_len);
            return ERRCODE_SUCC;
        }
    }
    return ERRCODE_FAIL;
}
#endif

errcode_t transmit_msg_proc(uint32_t msg_id, const uint8_t *msg, uint32_t msg_len)
{
    errcode_t ret = ERRCODE_FAIL;
    diag_dfx_transmit_rev_msg();

#if defined(CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK) && (CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK == DFX_YES)
    if (msg_id > DFX_MSG_ID_RESERVE_MAX) {
        return transmit_run_hook(msg_id, msg, msg_len);
    }
#endif

    switch (msg_id) {
        case DFX_MSG_ID_DIAG_PKT:
            ret = diag_msg_proc((uint16_t)msg_id, (uint8_t *)msg, msg_len);
            break;
        case DFX_MSG_ID_TRANSMIT_FILE:
            ret = transmit_defult_msg_proc(msg, msg_len);
            break;
        default:
            break;
    }
    return ret;
}

errcode_t uapi_transmit_register_msg_proc_hook(uint32_t msg_id_start, uint32_t msg_id_end, transmit_msg_proc_hook hook)
{
#if defined(CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK) && (CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK == DFX_YES)
    if (msg_id_start >= msg_id_end || hook == NULL) {
        return ERRCODE_FAIL;
    }
    transmit_msg_proc_t *msg_proc;
    for (int i = 0; i < TRANSMIT_MSG_PROC_MAX; i++) {
        msg_proc = &g_transmit_msg_proc[i];
        if (!((msg_id_end < msg_proc->id_start) || (msg_id_start > msg_proc->id_end))) {
            /* 有msg_id范围重叠 */
            return ERRCODE_FAIL;
        }
        if (msg_proc->hook == (uintptr_t)NULL) {
            msg_proc->id_start = msg_id_start;
            msg_proc->id_end = msg_id_end;
            msg_proc->hook = (uintptr_t)hook;
            return ERRCODE_SUCC;
        }
    }
    return ERRCODE_FAIL;
#else
    unused(msg_id_start);
    unused(msg_id_end);
    unused(hook);
    return ERRCODE_NOT_SUPPORT;
#endif
}

errcode_t uapi_transmit_unregister_msg_proc_hook(transmit_msg_proc_hook hook)
{
#if defined(CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK) && (CONFIG_DFX_SUPPORT_TRANSMIT_FILE_HOOK == DFX_YES)
    transmit_msg_proc_t *msg_proc;
    for (int i = 0; i < TRANSMIT_MSG_PROC_MAX; i++) {
        msg_proc = &g_transmit_msg_proc[i];
        if (((transmit_msg_proc_hook)msg_proc->hook) == hook) {
            msg_proc->hook = (uintptr_t)NULL;
            return ERRCODE_SUCC;
        }
    }
    return ERRCODE_FAIL;
#else
    unused(hook);
    return ERRCODE_NOT_SUPPORT;
#endif
}

#endif /* (defined(CONFIG_DFX_SUPPORT_TRANSMIT_FILE) && (CONFIG_DFX_SUPPORT_TRANSMIT_FILE == DFX_YES)) */
