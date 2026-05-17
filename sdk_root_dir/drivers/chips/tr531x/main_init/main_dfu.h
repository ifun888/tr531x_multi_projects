/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: Application core main dfu function \n
 *
 * History: \n
 * 2024-07-08, Create file. \n
 */

#ifndef MAIN_DFU
#define MAIN_DFU

#include "securec.h"
#include "gadget/f_hid.h"
#include "gadget/f_dfu.h"
#include "implementation/usb_init.h"
#include "partition.h"
#include "cpu_utils.h"
#include "osal_interrupt.h"
#include "watchdog.h"
#include "sfc.h"
#include "upg.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

void usb_hid_get_index(void);
int usb_hid_recv_data(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif