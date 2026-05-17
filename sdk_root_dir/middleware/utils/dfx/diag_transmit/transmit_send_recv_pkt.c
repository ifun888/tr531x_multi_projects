/*
 * Copyright (c) Triductor. 2021-2023. All rights reserved.
 * Description: transmit header file
 * This file should be changed only infrequently and with great care.
 */
#include "transmit_send_recv_pkt.h"
#include "diag.h"
#include "transmit_item.h"
#include "transmit_dst.h"
#include "transmit_src.h"
#include "transmit_debug.h"
#include "errcode.h"
#include "dfx_adapt_layer.h"
#include "diag_service.h"
#include "diag_pkt_router.h"

errcode_t transmit_receiver_start(uint8_t cmd_id, void *cmd_param, uint16_t cmd_param_size, diag_option_t *option)
{
    unused(cmd_id);
    unused(cmd_param_size);
    errcode_t ret = ERRCODE_FAIL;
    transmit_start_pkt_t *start_pkt = (transmit_start_pkt_t *)transmit_get_tlv_payload(cmd_param);

    /* START命令只会被下位机接收，可根据transmit_type判断是src还是dst */
    switch (start_pkt->transmit_type) {
        case TRANSMIT_TYPE_FILE_DOWNSTREAM:
        case TRANSMIT_TYPE_OTA_IMG_DOWNSTREAM:
        case TRANSMIT_TYPE_MEMORY_DOWNSTREAM:
        case TRANSMIT_TYPE_FLASH_DOWNSTREAM:
            ret = transmit_dst_item_process_start_frame(start_pkt, option);
            break;
        case TRANSMIT_TYPE_FILE_UPSTREAM:
        case TRANSMIT_TYPE_OTA_IMG_UPSTREAM:
        case TRANSMIT_TYPE_MEMORY_UPSTREAM:
        case TRANSMIT_TYPE_FLASH_UPSTREAM:
            ret = transmit_src_item_process_start_frame(start_pkt, option);
            break;
        default:
            break;
    }
    return ret;
}

errcode_t transmit_receiver_negotiate(uint8_t cmd_id, void *cmd_param, uint16_t cmd_param_size, diag_option_t *option)
{
    unused(cmd_id);
    unused(cmd_param_size);

#if (defined(CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE) && (CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE == DFX_YES))
    transmit_pkt_tlv_t *tlv = (transmit_pkt_tlv_t *)cmd_param;

    /* NEGOTIATE命令不仅下位机会接收到，还要考虑上位机接收到NEGOTIATE ACK命令的情况 */
    if ((tlv->type & 0x7F) == 0x02) { /* (type & 0x7F) == 0x02 means negotiate ack packet */
        transmit_negotiate_ack_pkt_t *nego_ack_pkt = (transmit_negotiate_ack_pkt_t*)transmit_get_tlv_payload(cmd_param);
        return transmit_dst_host_process_negotiate_ack_frame(nego_ack_pkt);
    }
#endif

    errcode_t ret = ERRCODE_FAIL;
    transmit_negotiate_pkt_t *negotiate_pkt = (transmit_negotiate_pkt_t *)transmit_get_tlv_payload(cmd_param);

    switch (negotiate_pkt->transmit_type) {
        case TRANSMIT_TYPE_FILE_DOWNSTREAM:
        case TRANSMIT_TYPE_OTA_IMG_DOWNSTREAM:
        case TRANSMIT_TYPE_MEMORY_DOWNSTREAM:
        case TRANSMIT_TYPE_FLASH_DOWNSTREAM:
            ret = transmit_dst_item_process_negotiate_frame(negotiate_pkt, option);
            break;
        case TRANSMIT_TYPE_FILE_UPSTREAM:
        case TRANSMIT_TYPE_MEMORY_UPSTREAM:
        case TRANSMIT_TYPE_FLASH_UPSTREAM:
        case TRANSMIT_TYPE_OTA_IMG_UPSTREAM:
            ret = transmit_src_item_process_negotiate_frame(negotiate_pkt, option);
            break;
        default:
            break;
    }
    return ret;
}

errcode_t transmit_receiver_data_request(uint8_t cmd_id, void *cmd_param, uint16_t cmd_param_size,
    diag_option_t *option)
{
    unused(cmd_id);
    unused(cmd_param_size);
    transmit_data_request_pkt_t *request_pkt = (transmit_data_request_pkt_t *)transmit_get_tlv_payload(cmd_param);
    return transmit_src_item_process_data_request_frame(request_pkt, option);
}

errcode_t transmit_receiver_data_reply(uint8_t cmd_id, void *cmd_param, uint16_t cmd_param_size, diag_option_t *option)
{
    unused(cmd_id);
    unused(cmd_param_size);
    transmit_data_reply_pkt_t *reply_pkt = (transmit_data_reply_pkt_t *)transmit_get_tlv_payload(cmd_param);
    return transmit_dst_item_process_data_reply_frame(reply_pkt, option);
}

errcode_t transmit_receiver_notify(uint8_t cmd_id, void *cmd_param, uint16_t cmd_param_size, diag_option_t *option)
{
    unused(cmd_id);
    unused(cmd_param_size);
    transmit_state_notify_pkt_t *notify_pkt = (transmit_state_notify_pkt_t *)transmit_get_tlv_payload(cmd_param);
    transmit_item_process_notify_frame(notify_pkt, option);
    return ERRCODE_SUCC;
}

errcode_t transmit_receiver_stop(uint8_t cmd_id, void *cmd_param, uint16_t cmd_param_size, diag_option_t *option)
{
    unused(cmd_id);
    unused(cmd_param_size);

    bool is_ack = false;
#if (defined(CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE) && (CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE == DFX_YES))
    transmit_pkt_tlv_t *tlv = (transmit_pkt_tlv_t *)cmd_param;
    /* 上位机才可能收到STOP ACK命令 */
    if ((tlv->type & 0x7F) == 0x02) { /* (type & 0x7F) == 0x02 means STOP ACK packet */
        is_ack = true;
    }
#endif

    transmit_stop_pkt_t *stop_pkt = (transmit_stop_pkt_t *)transmit_get_tlv_payload(cmd_param);
    transmit_item_process_stop_frame(stop_pkt, option, is_ack);
    return ERRCODE_SUCC;
}

errcode_t transmit_send_packet(uint8_t cmd_id, uint8_t *pkt, uint32_t pkt_size, diag_option_t *option)
{
    errcode_t ret;
    uint16_t buf_len = (uint16_t)sizeof(diag_ser_header_t) + (uint16_t)sizeof(diag_ser_frame_t) + (uint16_t)pkt_size;
    uint8_t *buf = dfx_malloc(0, buf_len);
    if (buf == NULL) {
        return ERRCODE_MALLOC;
    }

    diag_ser_header_t *header = (diag_ser_header_t *)buf;
    header->ser_id = DIAG_SER_FILE_TRANSMIT;
    header->cmd_id = cmd_id;
    header->src = DIAG_FRAME_FID_MCU;
    header->dst = option->peer_addr;

    header->crc_en = true;
    header->ack_en = false;
    header->length = buf_len - (uint16_t)sizeof(diag_ser_header_t);

    diag_ser_frame_t *frame = (diag_ser_frame_t *)(buf + sizeof(diag_ser_header_t));
    frame->module_id = DIAG_SER_FILE_TRANSMIT;
    frame->cmd_id = cmd_id;

    memcpy_s(((uint8_t *)frame + sizeof(diag_ser_frame_t)), pkt_size, pkt, pkt_size);

    transmit_printf_send_frame(cmd_id, pkt, (uint16_t)pkt_size, option);

    ret = uapi_diag_service_send_data((diag_ser_data_t *)buf);
    dfx_free(0, buf);
    return ret;
}
