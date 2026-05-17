/*
 * Copyright (c) Triductor. 2021-2021. All rights reserved.
 * Description: transmit data
 * This file should be changed only infrequently and with great care.
 */
#include "transmit_item.h"
#include "securec.h"
#include "transmit_st.h"
#include "transmit_send_recv_pkt.h"
#include "soc_osal.h"
#include "diag_adapt_layer.h"
#include "transmit_src.h"
#include "transmit_dst.h"
#include "transmit_host.h"
#include "transmit_msg.h"
#include "errcode.h"
#if CONFIG_DFX_SUPPORT_FILE_SYSTEM == DFX_YES
#include "dfx_file_operation.h"
#endif
#include "transmit_debug.h"
#include "transmit_cmd_id.h"
#include "transmit.h"

#define TRANSMIT_TIMER_PERIOD 1000
#define TRANSMIT_ITEM_BUF_LEN 0x600


STATIC void transmit_timer_stop(void);
static inline void transmit_timer_start(void);

STATIC errcode_t transmit_send(uint32_t transmit_id, diag_option_t *option, uint32_t code)
{
    errcode_t ret;
    uint32_t pkt_size = (uint32_t)sizeof(transmit_pkt_tlv_t) + (uint32_t)sizeof(transmit_state_notify_pkt_t);
    transmit_pkt_tlv_t *tlv = transmit_item_get_pkt_buf(NULL, pkt_size);
    if (tlv == NULL) {
        return ERRCODE_MALLOC;
    }
    tlv->type = transmit_build_tlv_type(1, 1);
    tlv->len = transmit_build_tlv_len(sizeof(transmit_state_notify_pkt_t));
    transmit_state_notify_pkt_t *pkt = (transmit_state_notify_pkt_t *)tlv->data;

    pkt->transmit_id = transmit_id;
    pkt->state_code = code;
    pkt->len = 0;
    ret = transmit_send_packet(DIAG_CMD_ID_TRANSMIT_NOTIFY, (uint8_t *)tlv, pkt_size, option);
    transmit_item_free_pkt_buf(NULL, tlv);
    return ret;
}

errcode_t transmit_send_invalid_id(uint32_t transmit_id, diag_option_t *option)
{
    return transmit_send(transmit_id, option, TRANSMIT_STATE_NOTIFY_INVALID_ID);
}

errcode_t transmit_send_finish_pkt(uint32_t transmit_id, diag_option_t *option)
{
    return transmit_send(transmit_id, option, TRANSMIT_STATE_NOTIFY_FINISH);
}

errcode_t transmit_send_failed_pkt(uint32_t transmit_id, diag_option_t *option)
{
    return transmit_send(transmit_id, option, TRANSMIT_STATE_NOTIFY_SAVE_FAILED_ID);
}

void transmit_item_process_notify_frame(transmit_state_notify_pkt_t *pkt, diag_option_t *option)
{
    dfx_assert(pkt);
    transmit_item_t *item = transmit_item_match_id(pkt->transmit_id);
    if (item == NULL) {
        if (pkt->state_code != TRANSMIT_STATE_NOTIFY_INVALID_ID) {
            dfx_log_err("[ERR]notify_frame match id failed!, id = 0x%x\r\n", pkt->transmit_id);
            transmit_send_invalid_id(pkt->transmit_id, option);
        }
        return;
    }

    switch (pkt->state_code) {
        case TRANSMIT_STATE_NOTIFY_INVALID_ID:
            transmit_item_finish(item, TRANSMIT_DISABLE_RECV_INVALID_ID);
            break;
        case TRANSMIT_STATE_NOTIFY_FINISH:
        case TRANSMIT_STATE_NOTIFY_FINISH_2:
            transmit_item_finish(item, TRANSMIT_DISABLE_RECV_FINISH);
            /* 在下位机收到FINISH命令时，需给上位机回复FINISH ACK */
            if (item->local_host == 0) {
                transmit_send_finish_pkt(item->transmit_id, &item->option);
            }
            break;
        case TRANSMIT_STATE_NOTIFY_DUPLICATE_ID:
            break;
        default:
            break;
    }
}

errcode_t transmit_item_process_stop_frame(transmit_stop_pkt_t *pkt, diag_option_t *option, bool is_ack)
{
    transmit_item_t *item = transmit_item_match_id(pkt->transmit_id);
    if (item == NULL) {
        dfx_log_debug("stop match id failed!, id = 0x%x\r\n", pkt->transmit_id);
        return transmit_send_invalid_id(pkt->transmit_id, option);
    }

    transmit_item_finish(item, TRANSMIT_DISABLE_USER_STOP);
    if (is_ack) {
        return ERRCODE_SUCC;
    }

    errcode_t ret;
    uint32_t pkt_size = (uint32_t)sizeof(transmit_pkt_tlv_t) + (uint32_t)sizeof(transmit_stop_pkt_t);
    transmit_pkt_tlv_t *tlv = transmit_item_get_pkt_buf(item, pkt_size);
    if (tlv == NULL) {
        return ERRCODE_MALLOC;
    }
    tlv->type =  transmit_build_tlv_type(1, 2); /* the type is assigned 2 in ack command */
    tlv->len = transmit_build_tlv_len(sizeof(transmit_stop_pkt_t));
    transmit_stop_pkt_t *stop_pkt = (transmit_stop_pkt_t *)tlv->data;

    stop_pkt->transmit_id = pkt->transmit_id;
    stop_pkt->reason = ERRCODE_SUCC;

    ret = transmit_send_packet(DIAG_CMD_ID_TRANSMIT_STOP, (uint8_t *)tlv, pkt_size, option);
    transmit_item_free_pkt_buf(item, tlv);
    return ret;
}

transmit_item_t *transmit_item_match_id(uint32_t transmit_id)
{
    transmit_item_t *item = NULL;
    for (int i = 0; i < CONFIG_DIAG_TRANSMIT_ITEM_CNT; i++) {
        item = &(transmit_get_ctrl()->item[i]);
        if ((item->used != 0) && (item->transmit_id == transmit_id) && (transmit_id != 0)) {
            return item;
        }
    }
    return NULL;
}

transmit_item_t *transmit_item_match_type_and_dst(transmit_type_t transmit_type, diag_addr dst)
{
    transmit_item_t *item = NULL;
    for (int i = 0; i < CONFIG_DIAG_TRANSMIT_ITEM_CNT; i++) {
        item = &(transmit_get_ctrl()->item[i]);
        if ((item->used != 0) && (item->transmit_type == transmit_type) && (item->option.peer_addr == dst)) {
            return item;
        }
    }
    return NULL;
}

transmit_item_t *transmit_item_init(uint32_t transmit_id)
{
    transmit_item_t *item = NULL;
    for (int i = 0; i < CONFIG_DIAG_TRANSMIT_ITEM_CNT; i++) {
        item = &(transmit_get_ctrl()->item[i]);
        if (item->used == true) {
            continue;
        }
        memset_s(item, sizeof(transmit_item_t), 0x0, sizeof(transmit_item_t));

        if (transmit_id == 0) {
            item->transmit_id = transmit_get_ctrl()->transmit_id++;
        } else {
            item->transmit_id = transmit_id;
        }

        item->used = true;
        if (item->pm_veto == 0) {
            (void)dfx_pm_add_sleep_veto();
            item->pm_veto = 1;
        }
        return item;
    }
    return NULL;
}

void transmit_item_init_local_file_name(transmit_item_t *item, const char *file_name, uint16_t name_len)
{
    dfx_assert(file_name);
    dfx_assert(item);
    uint32_t name_size = name_len + 1;
    item->local_file_name = dfx_malloc(0, name_size);
    if (item->local_file_name == NULL) {
        item->init_fail = true;
        return;
    }
    memcpy_s(item->local_file_name, name_len, file_name, name_len);
    item->local_file_name[name_len] = '\0';
}

#if (defined(CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE) && (CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE == DFX_YES))
void transmit_item_init_remote_file_name(transmit_item_t *item, const char *file_name, uint16_t name_len)
{
    dfx_assert(file_name);
    dfx_assert(item);
    uint32_t name_size = name_len + 1;
    item->remote_file_name = dfx_malloc(0, name_size);
    if (item->remote_file_name == NULL) {
        item->init_fail = true;
        return;
    }
    memcpy_s(item->remote_file_name, name_len, file_name, name_len);
    item->remote_file_name[name_len] = '\0';
}
#endif

void transmit_item_deinit(transmit_item_t *item)
{
    dfx_assert(item);
    if (item->used == false) {
        return;
    }
    if (item->local_file_name) {
        dfx_free(0, item->local_file_name);
        item->local_file_name = NULL;
    }
#if (defined(CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE) && (CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE == DFX_YES))
    if (item->remote_file_name) {
        dfx_free(0, item->remote_file_name);
        item->remote_file_name = NULL;
    }
#endif

#if CONFIG_DFX_SUPPORT_FILE_SYSTEM == DFX_YES
    if (item->file_fd != 0) {
        dfx_file_fsync(item->file_fd);
        dfx_file_close(item->file_fd);
        item->file_fd = 0;
    }
#endif

    if (item->pm_veto == 1) {
        (void)dfx_pm_remove_sleep_veto();
        item->pm_veto = 0;
    }

    item->used = false;
}

void transmit_item_disable(transmit_item_t *item, transmit_disable_reason_t reason)
{
    dfx_assert(item);
    if (item->enable == false) {
        return;
    }
    if (item->permanent == false) {
        transmit_get_ctrl()->timer_cnt--;
        if (transmit_get_ctrl()->timer_cnt == 0) {
            transmit_timer_stop();
        }
    }
    unused(reason);
    item->enable = false;
}

void transmit_item_enable(transmit_item_t *item)
{
    dfx_assert(item);
    uint32_t cur_time = dfx_get_cur_second();

    if (item->enable == true) {
        return;
    }

    if ((item->local_host) != 0) {
        item->step = TRANSMIT_STEP_START;
    } else {
        item->step = TRANSMIT_STEP_TRANSMIT;
    }

    if (item->permanent == false) {
        item->last_rcv_pkt_time = cur_time;
        item->last_send_pkt_time = cur_time;
        item->expiration = cur_time + TRANSMIT_RETRY_TIME;

        transmit_get_ctrl()->timer_cnt++;
        if (transmit_get_ctrl()->timer_cnt == 1) {
            transmit_timer_start();
        }
    }
    item->enable = true;
}

void transmit_item_finish(transmit_item_t *item, transmit_disable_reason_t reason)
{
    dfx_assert(item);

    transmit_item_disable(item, reason);

    errcode_t ret = ERRCODE_FAIL;
    switch (reason) {
        case TRANSMIT_DISABLE_TIME_OUT:
            ret = ERRCODE_DIAG_TRANSMIT_TIMEOUT;
            break;
        case TRANSMIT_DISABLE_RECV_FINISH:
        case TRANSMIT_DISABLE_RECV_ALL_DATA:
            ret = ERRCODE_SUCC;
            break;
        case TRANSMIT_DISABLE_USER_STOP:
            ret = ERRCODE_DIAG_TRANSMIT_USER_STOP;
            break;
        case TRANSMIT_DISABLE_RECV_INVALID_ID:
        default:
            break;
    }

    transmit_result_hook result_hook = (transmit_result_hook)item->result_hook;
    if (result_hook != NULL && item->used == true) {
        result_hook(ret, (uintptr_t)item->result_usr_data);
    }

    transmit_item_deinit(item);
}

void* transmit_item_get_pkt_buf(const transmit_item_t *item, uint32_t buf_size)
{
    dfx_assert(item);
    unused(item);

    if (transmit_get_ctrl()->pkt_size == 0) {
        transmit_get_ctrl()->pkt_size = TRANSMIT_ITEM_BUF_LEN;
        transmit_get_ctrl()->pkt_buf = dfx_malloc(0, TRANSMIT_ITEM_BUF_LEN);
    }

    if ((buf_size < transmit_get_ctrl()->pkt_size) && (transmit_get_ctrl()->buf_used == false)) {
        transmit_get_ctrl()->buf_used = true;
        (void)memset_s(transmit_get_ctrl()->pkt_buf, buf_size, 0, buf_size);
        return transmit_get_ctrl()->pkt_buf;
    }
    return NULL;
}

void transmit_item_free_pkt_buf(const transmit_item_t *item, const void *buf)
{
    transmit_get_ctrl()->buf_used = false;

    unused(item);
    unused(buf);
}

#if (CONFIG_DFX_SUPPORT_FILE_SYSTEM == DFX_YES)
errcode_t transmit_item_storage_init(transmit_item_t *item)
{
    if (item->local_file_name == NULL) {
        return ERRCODE_SUCC;
    }

    if (item->local_src == true) {
        item->file_fd = dfx_file_open_for_read((const char *)item->local_file_name);
    } else {
        item->file_fd = dfx_file_open_for_write((const char *)item->local_file_name);
    }
    if (item->file_fd < 0) {
        return ERRCODE_FAIL;
    }
    return ERRCODE_SUCC;
}
#endif

errcode_t transmit_item_stop_all_items(bool only_device)
{
    transmit_item_t *item = NULL;
    for (int i = 0; i < CONFIG_DIAG_TRANSMIT_ITEM_CNT; i++) {
        item = &(transmit_get_ctrl()->item[i]);
        if ((item->used == false) || ((only_device == true) && item->local_host != 0)) {
            continue;
        }

        if (item->enable) {
            uint32_t pkt_size = (uint32_t)sizeof(transmit_pkt_tlv_t) + (uint32_t)sizeof(transmit_stop_pkt_t);
            transmit_pkt_tlv_t *tlv = transmit_item_get_pkt_buf(item, pkt_size);
            if (tlv != NULL) {
                tlv->type =  transmit_build_tlv_type(1, 2); /* the type is assigned 2 in ack command */
                tlv->len = transmit_build_tlv_len(sizeof(transmit_stop_pkt_t));
                transmit_stop_pkt_t *stop_pkt = (transmit_stop_pkt_t *)tlv->data;

                stop_pkt->transmit_id = item->transmit_id;
                stop_pkt->reason = ERRCODE_SUCC;

                (void)transmit_send_packet(DIAG_CMD_ID_TRANSMIT_STOP, (uint8_t *)tlv, pkt_size, &(item->option));
                transmit_item_free_pkt_buf(item, tlv);
            }
        }
        transmit_item_finish(item, TRANSMIT_DISABLE_USER_STOP);
    }
    return ERRCODE_SUCC;
}

STATIC void transmit_timer_handler(unsigned long data)
{
    unused(data);
    osal_timer *timer = &(transmit_get_ctrl()->timer);
    transmit_msg_t msg;
    msg.msg_type = TRANSMIT_MSG_TYPE_TIMER;
    msg.msg_len = 0;
    transmit_msg_write(DFX_MSG_ID_TRANSMIT_FILE, (uint8_t *)(uintptr_t)&msg, sizeof(transmit_msg_t), false);

    if ((transmit_get_ctrl()->timer_cnt) != 0) {
        osal_timer_start(timer);
    }
}

static inline void transmit_timer_start(void)
{
    osal_timer *timer = &(transmit_get_ctrl()->timer);
    osal_timer_start(timer);
}

STATIC void transmit_timer_stop(void) {}
errcode_t transmit_timer_init(void)
{
    osal_timer *timer = &(transmit_get_ctrl()->timer);
    timer->handler = transmit_timer_handler;
    timer->data = 0;
    timer->interval = TRANSMIT_TIMER_PERIOD;
    if (osal_timer_init(timer) < 0) {
        return ERRCODE_FAIL;
    }
    return ERRCODE_SUCC;
}

errcode_t transmit_timer_deinit(void)
{
    osal_timer *timer = &(transmit_get_ctrl()->timer);
    if (osal_timer_destroy(timer) < 0) {
        return ERRCODE_FAIL;
    }
    return ERRCODE_SUCC;
}