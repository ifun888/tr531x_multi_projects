/*
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 * Description: transmit msg proc
 * This file should be changed only infrequently and with great care.
 */
#ifndef TRANSMIT_MSG_H
#define TRANSMIT_MSG_H

#include <stdint.h>
#include <stdbool.h>
#include "errcode.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

typedef enum {
    TRANSMIT_MSG_TYPE_TIMER,
    TRANSMIT_MSG_TYPE_HOST_START,
    TRANSMIT_MSG_TYPE_HOST_STOP,
    TRANSMIT_MSG_TYPE_DEVICE_STOP,
    TRANSMIT_MSG_TYPE_MAX,
} transmit_msg_type_t; /* transmit msg id */

typedef struct {
    uint8_t msg_type;
    uint8_t msg_len;
    uint8_t msg_data[0];
} transmit_msg_t;

typedef struct {
    uint32_t  id_start;
    uint32_t  id_end;
    uintptr_t hook;
} transmit_msg_proc_t;

errcode_t transmit_msg_proc(uint32_t msg_id, const uint8_t *msg, uint32_t msg_len);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* TRANSMIT_MSG_H */
