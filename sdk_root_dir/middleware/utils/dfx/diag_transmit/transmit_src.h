/*
 * Copyright (c) Triductor. 2021-2021. All rights reserved.
 * Description: transmit
 * This file should be changed only infrequently and with great care.
 */
#ifndef TRANSMIT_SRC_H
#define TRANSMIT_SRC_H

#include "transmit_item.h"

errcode_t transmit_src_item_process_data_request_frame(transmit_data_request_pkt_t *request_pkt, diag_option_t *option);
errcode_t transmit_src_item_process_start_frame(transmit_start_pkt_t *start_pkt, diag_option_t *option);
errcode_t transmit_src_item_process_negotiate_frame(transmit_negotiate_pkt_t *negotiate_pkt, diag_option_t *option);

void transmit_src_item_process_timer(transmit_item_t *item, uint32_t cur_time);

#endif /* TRANSMIT_SRC_H */
