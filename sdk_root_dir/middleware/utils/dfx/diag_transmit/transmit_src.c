/*
 * Copyright (c) Triductor. 2021-2021. All rights reserved.
 * Description: transmit data
 * This file should be changed only infrequently and with great care.
 */
#include "transmit_src.h"
#include "transmit_dst.h"
#include "transmit_st.h"
#include "transmit_item.h"
#include "transmit_host.h"
#include "transmit_send_recv_pkt.h"
#include "transmit_write_read.h"
#include "dfx_feature_config.h"
#include "diag_adapt_layer.h"
#if CONFIG_DFX_SUPPORT_FILE_SYSTEM == DFX_YES
#include "dfx_file_operation.h"
#endif
#include "transmit_debug.h"
#include "transmit_cmd_id.h"
#include "string.h"
#include "securec.h"
#include "errcode.h"

STATIC uint32_t transmit_data_read(transmit_item_t *item, uint32_t offset, uint8_t *buf, uint32_t size)
{
    transmit_read_hook read_handler = (transmit_read_hook)item->write_read;
    return (uint32_t)read_handler(item->usr_wr_data, offset, buf, size);
}

/* 作为源端（读取数据并发送数据的一端）读取并发送数据 */
STATIC errcode_t transmit_process_data_request_one(transmit_item_t *item, uint32_t offset, uint32_t size,
                                                   diag_option_t *option)
{
    transmit_data_reply_pkt_t *pkt = NULL;

    uint32_t data_size;
    uint32_t tlv_ext_len;
    uint32_t pkt_size;
    int32_t send_size = 0;
    int32_t left_size = (int32_t)size;
    int32_t read_size, readed_size;
    uint16_t block_size = (item->data_block_size == 0) ? DEFAULT_TRANSMIT_BLOCK_SIZE : item->data_block_size;
    transmit_pkt_tlv_t *tlv = transmit_item_get_pkt_buf(item,
        sizeof(transmit_pkt_tlv_t) + sizeof(uint16_t) + sizeof(transmit_data_reply_pkt_t) + block_size);
    if (tlv == NULL) {
        return ERRCODE_MALLOC;
    }

    while ((left_size) != 0) {
        read_size = uapi_min(left_size, block_size);
        data_size = (uint32_t)sizeof(transmit_data_reply_pkt_t) + (uint32_t)read_size;
        tlv_ext_len = transmit_get_tlv_ext_len(data_size);
        pkt_size = (uint32_t)sizeof(transmit_pkt_tlv_t) + tlv_ext_len + data_size;

        tlv->type =  transmit_build_tlv_type(1, 1);
        tlv->len = transmit_build_tlv_len(data_size);
        if (tlv_ext_len > 0) {
            (void)memcpy_s(tlv->data, tlv_ext_len, ((uint8_t *)&data_size), tlv_ext_len);
        }

        pkt = (transmit_data_reply_pkt_t *)((uint8_t *)tlv + sizeof(transmit_pkt_tlv_t) + tlv_ext_len);

        (void)memset_s(pkt->data, (uint32_t)read_size, 0, (uint32_t)read_size);
        readed_size = (int32_t)transmit_data_read(item, offset + send_size, pkt->data, (uint32_t)read_size);
        if (readed_size <= 0) {
            dfx_log_err("[ERR][transmit src] read data failed!, offset = 0x%x, read_size = %d readed_size = %d\r\n",
                offset + send_size, read_size, readed_size);
            transmit_send_failed_pkt(item->transmit_id, &item->option);
            goto end;
        }

        pkt->transmit_id = item->transmit_id;
        pkt->offset = offset + (uint32_t)send_size;
        pkt->size = (uint32_t)readed_size;

        transmit_send_packet(DIAG_CMD_ID_TRANSMIT_REPLY, (uint8_t *)tlv, pkt_size, option);

        left_size -= readed_size;
        send_size += readed_size;
    }
end:
    transmit_item_free_pkt_buf(item, (void *)tlv);
    return ERRCODE_SUCC;
}

/* 作为源端（读取数据并发送数据的一端）发送NEGOTIATE(协商)ACK帧 */
STATIC void transmit_src_item_send_negotiate_ack(transmit_item_t *item)
{
    uint32_t pkt_size = (uint32_t)sizeof(transmit_pkt_tlv_t) + (uint32_t)sizeof(transmit_negotiate_ack_pkt_t);
    transmit_pkt_tlv_t *tlv = transmit_item_get_pkt_buf(item, pkt_size);
    if (tlv == NULL) {
        return;
    }
    tlv->type =  transmit_build_tlv_type(1, 2); /* the type is assigned 2 in ack command */
    tlv->len = transmit_build_tlv_len(sizeof(transmit_negotiate_ack_pkt_t));
    transmit_negotiate_ack_pkt_t *pkt = (transmit_negotiate_ack_pkt_t *)tlv->data;

    pkt->transmit_id = item->transmit_id;
    pkt->data_block_number = item->data_block_number;
    pkt->data_block_size = item->data_block_size;
    pkt->info_size = 0;

    (void)transmit_send_packet(DIAG_CMD_ID_TRANSMIT_NEGOTIATE, (uint8_t *)tlv, pkt_size, &item->option);
    transmit_item_free_pkt_buf(item, (void *)tlv);
}

STATIC void transmit_src_item_send_negotiate(transmit_item_t *item)
{
    uint32_t pkt_size = (uint32_t)sizeof(transmit_pkt_tlv_t) + (uint32_t)sizeof(transmit_negotiate_pkt_t);
    transmit_pkt_tlv_t *tlv = transmit_item_get_pkt_buf(item, pkt_size);
    if (tlv == NULL) {
        return;
    }
    tlv->type =  transmit_build_tlv_type(1, 1);
    tlv->len = transmit_build_tlv_len(sizeof(transmit_negotiate_pkt_t));
    transmit_negotiate_pkt_t *pkt = (transmit_negotiate_pkt_t *)tlv->data;

    (void)memset_s((uint8_t *)pkt, sizeof(transmit_negotiate_pkt_t), 0, sizeof(transmit_negotiate_pkt_t));
    pkt->transmit_id = item->transmit_id;
    pkt->src_send = 0;
    pkt->transmit_type = item->transmit_type;
    pkt->total_size = item->total_size;

    (void)transmit_send_packet(DIAG_CMD_ID_TRANSMIT_NEGOTIATE, (uint8_t *)tlv, pkt_size, &item->option);
    transmit_item_free_pkt_buf(item, (void *)tlv);
}

STATIC transmit_item_t *transmit_src_item_init(uint32_t transmit_id)
{
    transmit_item_t *item = transmit_item_match_id(transmit_id);
    if (item != NULL) {
        transmit_item_disable(item, 0);
        transmit_item_deinit(item);
    }

    item = transmit_item_init(transmit_id);
    return item;
}

STATIC void transmit_src_item_init_info(transmit_item_t *item, uint16_t transmit_type,
                                        uint32_t total_size, bool re_trans)
{
    unused(re_trans);
    transmit_item_init_permanent(item, false);
    transmit_item_init_local_start(item, false);
    transmit_item_init_local_src(item, true);
    transmit_item_init_transmit_type(item, transmit_type);

    transmit_item_init_read_handler(item, file_read_data, (uintptr_t)item);
    transmit_item_init_total_size(item, total_size);
}

STATIC errcode_t transmit_src_item_start_negotiate_ack(transmit_item_t *item, bool start_ack)
{
    uint32_t cur_time = dfx_get_cur_second();

    if (transmit_item_init_is_success(item) == false) {
        transmit_item_deinit(item);
        dfx_log_err("[ERR][transmit src]init is failed\r\n");
        return ERRCODE_FAIL;
    }

#if (CONFIG_DFX_SUPPORT_FILE_SYSTEM == DFX_YES)
    if (transmit_item_storage_init(item) != ERRCODE_SUCC) {
        dfx_log_err("[ERR][transmit src]file : %s open failed, fd = %d\r\n", item->local_file_name, item->file_fd);
        transmit_send_failed_pkt(item->transmit_id, &item->option);
        transmit_item_deinit(item);
        return ERRCODE_FAIL;
    }
#endif

    transmit_item_enable(item);

    item->last_rcv_pkt_time = cur_time;
    item->last_send_pkt_time = cur_time;

    if (start_ack && item->option.peer_addr == DIAG_FRAME_FID_UART) {
        /* 为兼容PC工具，对于收到的START命令，回复NEGOTIATE帧数据 */
        transmit_src_item_send_negotiate(item);
    } else {
        /* 对于收到的NEGOTIATE命令，回复NEGOTIATE ACK帧数据 */
        transmit_src_item_send_negotiate_ack(item);
    }

    return ERRCODE_SUCC;
}

/* 作为源端（读取数据并发送数据的一端）处理REQUEST帧 */
errcode_t transmit_src_item_process_data_request_frame(transmit_data_request_pkt_t *request_pkt, diag_option_t *option)
{
    uint32_t i;
    uint32_t cur_time = dfx_get_cur_second();
    transmit_item_t *item = transmit_item_match_id(request_pkt->transmit_id);
    if (item == NULL) {
        dfx_log_err("[ERR][transmit src]request match id failed!, id = 0x%x\r\n", request_pkt->transmit_id);
        return transmit_send_invalid_id(request_pkt->transmit_id, option);
    }

    item->last_rcv_pkt_time = cur_time;
    item->last_send_pkt_time = cur_time;
    if ((item->local_host != 0) && (item->step == TRANSMIT_STEP_START)) {
        item->step = TRANSMIT_STEP_TRANSMIT;
    }

    for (i = 0; i < request_pkt->cnt; i++) {
        transmit_process_data_request_one(item, request_pkt->item[i].offset, request_pkt->item[i].size, option);
    }
    return ERRCODE_SUCC;
}

/* 作为源端（读取数据并发送数据的一端）处理START帧 */
errcode_t transmit_src_item_process_start_frame(transmit_start_pkt_t *start_pkt, diag_option_t *option)
{
    dfx_assert(start_pkt);
    transmit_save_info_t *file_info = (transmit_save_info_t *)start_pkt->info;
    transmit_save_data_info_t *data_info = (transmit_save_data_info_t *)start_pkt->info;

    transmit_item_t *item = transmit_src_item_init(start_pkt->transmit_id);
    if (item == NULL) {
        dfx_log_err("[ERR][transmit src]init failed\r\n");
        return ERRCODE_FAIL;
    }

    transmit_src_item_init_info(item, start_pkt->transmit_type, start_pkt->total_size, start_pkt->re_trans);
    switch (start_pkt->transmit_type) {
        case TRANSMIT_TYPE_FILE_UPSTREAM:
            transmit_item_init_local_file_name(item, file_info->file_name, file_info->name_size);
            break;
        case TRANSMIT_TYPE_FLASH_UPSTREAM:
        case TRANSMIT_TYPE_MEMORY_UPSTREAM:
            transmit_item_init_local_bus_addr(item, data_info->start_addr);
            break;
        default:
            break;
    }
    transmit_item_init_option(item, option);

    transmit_item_init_data_block_size(item, DEFAULT_TRANSMIT_BLOCK_SIZE);
    transmit_item_init_data_block_number(item, DEFAULT_TRANSMIT_BLOCK_NUMBER);

    transmit_ctrl_t *transmit_ctrl = transmit_get_ctrl();
    transmit_item_init_result_handler(item, (transmit_result_hook)transmit_ctrl->result_hook,
        transmit_ctrl->result_usr_data);

    return transmit_src_item_start_negotiate_ack(item, true);
}

/* 作为源端（读取数据并发送数据的一端）处理NEGOTIATE帧 */
errcode_t transmit_src_item_process_negotiate_frame(transmit_negotiate_pkt_t *negotiate_pkt, diag_option_t *option)
{
    dfx_assert(negotiate_pkt);
    transmit_save_info_t *file_info = (transmit_save_info_t *)negotiate_pkt->info;
    transmit_save_data_info_t *data_info = (transmit_save_data_info_t *)negotiate_pkt->info;

    transmit_item_t *item = transmit_src_item_init(negotiate_pkt->transmit_id);
    if (item == NULL) {
        dfx_log_err("[ERR][transmit src]init failed\r\n");
        return ERRCODE_FAIL;
    }

    transmit_src_item_init_info(item, negotiate_pkt->transmit_type, negotiate_pkt->total_size, negotiate_pkt->re_trans);

    switch (negotiate_pkt->transmit_type) {
        case TRANSMIT_TYPE_FILE_UPSTREAM:
            transmit_item_init_local_file_name(item, file_info->file_name, file_info->name_size);
            break;
        case TRANSMIT_TYPE_FLASH_UPSTREAM:
        case TRANSMIT_TYPE_MEMORY_UPSTREAM:
            transmit_item_init_local_bus_addr(item, data_info->start_addr);
            break;
        default:
            break;
    }

    transmit_item_init_option(item, option);

    transmit_item_init_data_block_size(item, negotiate_pkt->data_block_size);
    transmit_item_init_data_block_number(item, negotiate_pkt->data_block_number);

    transmit_ctrl_t *transmit_ctrl = transmit_get_ctrl();
    transmit_item_init_result_handler(item, (transmit_result_hook)transmit_ctrl->result_hook,
        transmit_ctrl->result_usr_data);

    return transmit_src_item_start_negotiate_ack(item, false);
}

void transmit_src_item_process_timer(transmit_item_t *item, uint32_t cur_time)
{
    uint32_t out_time = item->last_rcv_pkt_time + TRANSMIT_OUT_TIME;

    if (cur_time > out_time) {
        transmit_item_finish(item, TRANSMIT_DISABLE_TIME_OUT);
        return;
    }

#if (CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE == DFX_YES)
    uint32_t retry_time = uapi_min(item->last_send_pkt_time, item->last_rcv_pkt_time) + TRANSMIT_RETRY_TIME;
    if ((item->local_host != 0) && (item->step == TRANSMIT_STEP_START) && (cur_time > retry_time)) {
        /* send pkt and modify retry time */
        (void)transmit_host_send_negotiate_frame(item, cur_time);
    }
#endif
}