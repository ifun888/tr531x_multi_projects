/*
 * Copyright (c) Triductor. 2021-2024. All rights reserved.
 * Description: transmit data
 * This file should be changed only infrequently and with great care.
 */
#include "transmit_host.h"
#include "transmit_item.h"
#include "transmit_send_recv_pkt.h"
#include "transmit_src.h"
#include "transmit_dst.h"
#include "transmit_write_read.h"
#include "transmit_cmd_id.h"
#include "diag_common.h"
#include "transmit_debug.h"
#include "diag_adapt_layer.h"
#include "dfx_feature_config.h"

#if (defined(CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE) && (CONFIG_DFX_SUPPORT_DIAG_UP_MACHINE == DFX_YES))

errcode_t transmit_host_send_negotiate_frame(transmit_item_t *item, uint32_t cur_time)
{
    errcode_t ret = ERRCODE_FAIL;

    transmit_negotiate_pkt_t *negotiate_pkt = NULL;
    uint32_t name_size = 0;
    uint32_t info_size = 0;
    if (item->remote_file_name != NULL) {
        name_size = (uint32_t)strlen(item->remote_file_name) + 1;
        info_size = (uint32_t)sizeof(transmit_save_info_t) + name_size;
    } else if (item->remote_bus_addr != 0) {
        info_size = (uint32_t)sizeof(transmit_save_data_info_t);
    }
    uint32_t data_size = (uint32_t)sizeof(transmit_negotiate_pkt_t) + info_size;
    uint32_t tlv_ext_len = transmit_get_tlv_ext_len(data_size);
    uint32_t pkt_size = (uint32_t)sizeof(transmit_pkt_tlv_t) + tlv_ext_len + data_size;
    transmit_pkt_tlv_t *tlv = transmit_item_get_pkt_buf(item, pkt_size);
    if (tlv == NULL) {
        ret = ERRCODE_MALLOC;
        goto end;
    }

    tlv->type =  transmit_build_tlv_type(1, 1);
    tlv->len = transmit_build_tlv_len(data_size);
    if (tlv_ext_len > 0) {
        (void)memcpy_s(tlv->data, tlv_ext_len, ((uint8_t *)&data_size), tlv_ext_len);
    }

    negotiate_pkt = (transmit_negotiate_pkt_t *)((uint8_t *)tlv + sizeof(transmit_pkt_tlv_t) + tlv_ext_len);
    if (item->remote_file_name != NULL) {
        transmit_save_info_t *save_file_info = (transmit_save_info_t *)negotiate_pkt->info;
        save_file_info->name_size = (uint16_t)name_size;
        (void)memcpy_s(save_file_info->file_name, name_size, item->remote_file_name, name_size);
    } else if (item->remote_bus_addr != 0) {
        transmit_save_data_info_t *data_info = (transmit_save_data_info_t *)negotiate_pkt->info;
        data_info->start_addr = item->remote_bus_addr;
    }

    negotiate_pkt->transmit_id = item->transmit_id;
    negotiate_pkt->transmit_type = item->transmit_type;
    negotiate_pkt->total_size = item->total_size;
    negotiate_pkt->info_size = info_size;
    negotiate_pkt->re_trans = item->re_trans;
    negotiate_pkt->src_send = item->local_src;
    negotiate_pkt->data_block_number = item->data_block_number;
    negotiate_pkt->data_block_size = item->data_block_size;
    ret = transmit_send_packet(DIAG_CMD_ID_TRANSMIT_NEGOTIATE, (uint8_t *)tlv, pkt_size, &item->option);

    item->last_send_pkt_time = cur_time;
end:
    transmit_item_free_pkt_buf(item, tlv);
    return ret;
}

STATIC errcode_t transmit_host_init_item_by_transmit_type(transmit_item_t *item, transmit_cfg_info_t *cfg_info)
{
    switch (item->transmit_type) {
        case TRANSMIT_TYPE_FILE_DOWNSTREAM:
        case TRANSMIT_TYPE_OTA_IMG_DOWNSTREAM:
        case TRANSMIT_TYPE_MEMORY_DOWNSTREAM:
        case TRANSMIT_TYPE_FLASH_DOWNSTREAM:
            /* 数据下行，上位机为源端，读数据 */
            transmit_item_init_local_src(item, true);
            transmit_item_init_read_handler(item, file_read_data, (uintptr_t)item);
            break;
        case TRANSMIT_TYPE_FILE_UPSTREAM:
        case TRANSMIT_TYPE_MEMORY_UPSTREAM:
        case TRANSMIT_TYPE_FLASH_UPSTREAM:
        case TRANSMIT_TYPE_OTA_IMG_UPSTREAM:
            /* 数据下行，上位机为目的端，写数据 */
            transmit_item_init_local_src(item, false);
            transmit_item_init_write_handler(item, file_write_data, (uintptr_t)item);
            break;
        default:
            break;
    }

    switch (item->transmit_type) {
        case TRANSMIT_TYPE_FILE_DOWNSTREAM:
        case TRANSMIT_TYPE_FILE_UPSTREAM:
            transmit_item_init_local_file_name(item, cfg_info->data.file_info.host_file_name,
                strlen(cfg_info->data.file_info.host_file_name));
            transmit_item_init_remote_file_name(item, cfg_info->data.file_info.device_file_name,
                strlen(cfg_info->data.file_info.device_file_name));
            break;
        case TRANSMIT_TYPE_MEMORY_DOWNSTREAM:
        case TRANSMIT_TYPE_MEMORY_UPSTREAM:
        case TRANSMIT_TYPE_FLASH_DOWNSTREAM:
        case TRANSMIT_TYPE_FLASH_UPSTREAM:
            transmit_item_init_local_bus_addr(item, cfg_info->data.addr_info.host_start_addr);
            transmit_item_init_remote_bus_addr(item, cfg_info->data.addr_info.device_start_addr);
            break;
        case TRANSMIT_TYPE_OTA_IMG_DOWNSTREAM:
        case TRANSMIT_TYPE_OTA_IMG_UPSTREAM:
            break;
        default:
            return ERRCODE_INVALID_PARAM;
    }
    return ERRCODE_SUCC;
}

/* 作为上位机发送START帧 */
errcode_t transmit_host_send_start_cmd(transmit_type_t transmit_type, uint16_t channel_id,
    transmit_cfg_info_t *cfg_info, transmit_callback_t *callback)
{
    diag_option_t option = DIAG_OPTION_INIT_VAL;
    option.peer_addr = (diag_addr)channel_id;

    if (cfg_info == NULL || callback == NULL) {
        return ERRCODE_INVALID_PARAM;
    }

    transmit_item_t *item = transmit_item_match_type_and_dst(transmit_type, option.peer_addr);
    if (item != NULL) {
        transmit_item_disable(item, 0);
        transmit_item_deinit(item);
    }

    item = transmit_item_init(0);
    if (item == NULL) {
        return ERRCODE_FAIL;
    }

    transmit_item_init_transmit_type(item, transmit_type);
    transmit_item_init_permanent(item, false);
    transmit_item_init_local_start(item, true); /* 本地设为上位机 */
    transmit_item_init_option(item, &option);
    transmit_item_init_total_size(item, cfg_info->total_size);
    transmit_item_init_result_handler(item, callback->result_hook, callback->result_usr_data);
    transmit_item_init_re_trans(item, cfg_info->re_transmit);
    transmit_item_init_data_block_size(item, cfg_info->data_block_size);
    transmit_item_init_data_block_number(item, cfg_info->data_block_number);

    if (transmit_host_init_item_by_transmit_type(item, cfg_info) != ERRCODE_SUCC) {
        transmit_item_deinit(item);
        return ERRCODE_INVALID_PARAM;
    }

    if (transmit_item_init_is_success(item) == false) {
        transmit_item_deinit(item);
        return ERRCODE_FAIL;
    }

#if (CONFIG_DFX_SUPPORT_FILE_SYSTEM == DFX_YES)
    /* 在上位机中，作为源端时，需要事先打开文件，作为目的端时，在request命令时打开文件 */
    if (item->local_src == true) {
        if (transmit_item_storage_init(item) != ERRCODE_SUCC) {
            dfx_log_err("[ERR][transmit host]file : %s open failed, fd = %d\r\n", item->local_file_name, item->file_fd);
            transmit_item_deinit(item);
            return ERRCODE_FAIL;
        }
    }
#endif

    transmit_item_enable(item);
    return transmit_host_send_negotiate_frame(item, dfx_get_cur_second());
}

/* 作为上位机发送STOP帧 */
errcode_t transmit_host_send_stop_cmd(transmit_type_t transmit_type, uint16_t channel_id)
{
    errcode_t ret;
    diag_option_t option = DIAG_OPTION_INIT_VAL;
    option.peer_addr = (diag_addr)channel_id;
    transmit_item_t *item = transmit_item_match_type_and_dst(transmit_type, option.peer_addr);
    if (item == NULL) {
        dfx_log_debug("stop match id failed!, type = 0x%x, dst = 0x%x\r\n", transmit_type, option.peer_addr);
        (void)transmit_send_invalid_id(0, &option);
        return ERRCODE_INVALID_PARAM;
    }

    transmit_item_finish(item, TRANSMIT_DISABLE_USER_STOP);

    uint32_t pkt_size = (uint32_t)sizeof(transmit_pkt_tlv_t) + (uint32_t)sizeof(transmit_stop_pkt_t);
    transmit_pkt_tlv_t *tlv = transmit_item_get_pkt_buf(item, pkt_size);
    if (tlv == NULL) {
        return ERRCODE_MALLOC;
    }
    tlv->type =  transmit_build_tlv_type(1, 2); /* the type is assigned 2 in ack command */
    tlv->len = transmit_build_tlv_len(sizeof(transmit_stop_pkt_t));
    transmit_stop_pkt_t *stop_pkt = (transmit_stop_pkt_t *)tlv->data;

    stop_pkt->transmit_id = item->transmit_id;
    stop_pkt->reason = ERRCODE_SUCC;

    ret = transmit_send_packet(DIAG_CMD_ID_TRANSMIT_STOP, (uint8_t *)tlv, pkt_size, &option);
    transmit_item_free_pkt_buf(item, tlv);
    return ret;
}
#endif
