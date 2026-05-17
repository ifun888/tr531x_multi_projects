/*
 * Copyright (c) Triductor. 2021-2024. All rights reserved.
 * Description: transmit data
 * This file should be changed only infrequently and with great care.
 */
#ifndef TRANSMIT_HOST_H
#define TRANSMIT_HOST_H

#include "errcode.h"
#include "transmit.h"
#include "transmit_item.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */


typedef struct {
    transmit_type_t transmit_type;
    uint16_t channel_id;
    transmit_cfg_info_t cfg_info;
    transmit_callback_t callback;
} transmit_host_start_param_t;

typedef struct {
    transmit_type_t transmit_type;
    uint16_t channel_id;
} transmit_host_stop_param_t;

errcode_t transmit_host_send_negotiate_frame(transmit_item_t *item, uint32_t cur_time);

errcode_t transmit_host_send_start_cmd(transmit_type_t transmit_type, uint16_t channel_id,
    transmit_cfg_info_t *cfg_info, transmit_callback_t *callback);

errcode_t transmit_host_send_stop_cmd(transmit_type_t transmit_type, uint16_t channel_id);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* TRANSMIT_HOST_H */