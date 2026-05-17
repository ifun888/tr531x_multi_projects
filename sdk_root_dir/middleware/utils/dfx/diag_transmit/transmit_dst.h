/*
 * Copyright (c) Triductor. 2021-2021. All rights reserved.
 * Description: transmit
 * This file should be changed only infrequently and with great care.
 */
#ifndef TRANSMIT_DST_H
#define TRANSMIT_DST_H

#include <stdint.h>
#include "transmit_st.h"
#include "transmit_item.h"

errcode_t transmit_dst_item_process_data_reply_frame(transmit_data_reply_pkt_t *reply, diag_option_t *option);
errcode_t transmit_dst_item_process_start_frame(transmit_start_pkt_t *start_pkt, diag_option_t *option);
errcode_t transmit_dst_item_process_negotiate_frame(transmit_negotiate_pkt_t *negotiate_pkt, diag_option_t *option);
errcode_t transmit_dst_host_process_negotiate_ack_frame(transmit_negotiate_ack_pkt_t *negotiate_pkt);

void transmit_dst_item_process_timer(transmit_item_t *item, uint32_t cur_time);
void transmit_dst_item_send_data_request_frame(transmit_item_t *item, uint32_t cur_time);

#endif /* TRANSMIT_DST_H */
