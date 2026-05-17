/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: client header. \n
 * Author: Triductor \n
 * History: \n
 * 2023-04-03, Create file. \n
 */
#ifndef CLIENT_H
#define CLIENT_H

#include "sle_ssap_client.h"

ssapc_write_param_t get_g_sle_performance_send_param(void);
void sle_client_init(ssapc_notification_callback notification_cb, ssapc_indication_callback indication_cb);

#endif
