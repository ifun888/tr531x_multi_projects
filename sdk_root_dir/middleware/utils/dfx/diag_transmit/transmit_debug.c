/*
 * Copyright (c) Triductor. 2021-2021. All rights reserved.
 * Description: transmit
 * This file should be changed only infrequently and with great care.
 */
#include "transmit_debug.h"
#include "transmit_cmd_id.h"
#include "transmit_send_recv_pkt.h"

#if defined DEBUG_TRANSMIT
void transmit_printf_item(char *info, transmit_item_t *item)
{
    dfx_assert(info);
    dfx_assert(item);
    dfx_log_debug("++++++++++++++++++++%s start++++++++++++++++++++\r\n", info);
    dfx_log_debug("transmit_id=0x%x\r\n", item->transmit_id);
    dfx_log_debug("used=0x%x\r\n", item->used);
    dfx_log_debug("init_fail=0x%x\r\n", item->init_fail);
    dfx_log_debug("permanent=0x%x\r\n", item->permanent);
    dfx_log_debug("local_src=0x%x\r\n", item->local_src);
    dfx_log_debug("transmit_type=0x%x\r\n", item->transmit_type);
    dfx_log_debug("received_size=0x%x\r\n", item->received_size);
    dfx_log_debug("total_size=0x%x\r\n", item->total_size);
    dfx_log_debug("write_read=0x%x\r\n", item->write_read);
    dfx_log_debug("usr_wr_data=0x%x\r\n", item->usr_wr_data);
    dfx_log_debug("local_bus_addr=0x%x\r\n", item->local_bus_addr);
    dfx_log_debug("expiration=0x%x\r\n", item->expiration);
    dfx_log_debug("last_rcv_pkt_time=0x%x\r\n", item->last_rcv_pkt_time);
    dfx_log_debug("last_send_pkt_time=0x%x\r\n", item->last_send_pkt_time);
    dfx_log_debug("option=0x%x\r\n", item->option.peer_addr);
    if (item->local_file_name) {
        dfx_log_debug("local_file_name=%s\r\n", item->local_file_name);
    }
    dfx_log_debug("--------------------%s start--------------------\r\n", info);
    unused(info);
}

STATIC void transmit_printf_receive_data_start(void *cmd_param)
{
    transmit_start_pkt_t *start = (transmit_start_pkt_t *)transmit_get_tlv_payload(cmd_param);
    dfx_log_debug("[RECEIVER_START][id=%d][src_send=%d][transmit_type=0x%x][total_size=0x%x]\r\n", start->transmit_id,
        start->src_send, start->transmit_type, start->total_size);
}

STATIC void transmit_printf_receive_data_negotiate(void *cmd_param)
{
    transmit_negotiate_pkt_t *nego = (transmit_negotiate_pkt_t *)transmit_get_tlv_payload(cmd_param);
    dfx_log_debug("[RECEIVER_NEGO][id=%d][src_send=%d][type=0x%x][total_size=0x%x][block_mum=%d][block_size=%d]\r\n",
        nego->transmit_id, nego->src_send, nego->transmit_type, nego->total_size, nego->data_block_number,
        nego->data_block_size);
}

STATIC void transmit_printf_receive_data_request(void *cmd_param)
{
    transmit_data_request_pkt_t *req = (transmit_data_request_pkt_t *)transmit_get_tlv_payload(cmd_param);
    dfx_log_debug("[RECEIVER_REQUEST][id=%d][cnt=%d][0ffset=0x%x][size=0x%x]\r\n", req->transmit_id, req->cnt,
        req->item[0].offset, req->item[0].size);
    unused(req);
}

STATIC void transmit_printf_receive_data_reply(void *cmd_param)
{
    transmit_data_reply_pkt_t *reply = (transmit_data_reply_pkt_t *)transmit_get_tlv_payload(cmd_param);
    dfx_log_debug("[RECEIVER_REPLY][id=%d][0ffset=0x%x][size=0x%x]\r\n", reply->transmit_id,
        reply->offset, reply->size);
    unused(reply);
}

STATIC void transmit_printf_receive_notify(void *cmd_param)
{
    transmit_state_notify_pkt_t *notify = (transmit_state_notify_pkt_t *)transmit_get_tlv_payload(cmd_param);
    dfx_log_debug("[RECEIVER_NOTIFY][id=%d][code=%d][len=0x%x]\r\n", notify->transmit_id, notify->state_code,
        notify->len);
    unused(notify);
}

void transmit_printf_receive_frame(uint16_t cmd_id, void *cmd_param, uint16_t cmd_param_size, diag_option_t *option)
{
    unused(cmd_param_size);
    unused(option);

    switch (cmd_id) {
        case DIAG_CMD_ID_TRANSMIT_REQUEST:
            transmit_printf_receive_data_request(cmd_param);
            break;
        case DIAG_CMD_ID_TRANSMIT_REPLY:
            transmit_printf_receive_data_reply(cmd_param);
            break;
        case DIAG_CMD_ID_TRANSMIT_START:
            transmit_printf_receive_data_start(cmd_param);
            break;
        case DIAG_CMD_ID_TRANSMIT_NEGOTIATE:
            transmit_printf_receive_data_negotiate(cmd_param);
            break;
        case DIAG_CMD_ID_TRANSMIT_NOTIFY:
            transmit_printf_receive_notify(cmd_param);
            break;
        default:
            break;
    }
}

void transmit_printf_send_frame(uint16_t cmd_id, void *cmd_param, uint16_t cmd_param_size, diag_option_t *option)
{
    unused(cmd_id);
    unused(cmd_param);
    unused(cmd_param_size);
    unused(option);

    dfx_log_debug("[SEND FRAME][06 %02x]\r\n", cmd_id);
}
#endif
