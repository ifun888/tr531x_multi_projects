/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: USB Initialize Header. \n
 * Author: Triductor \n
 * History: \n
 * 2023-07-07, Create file. \n
 */
#ifndef TEST_USB_CUSTOM_H
#define TEST_USB_CUSTOM_H

#include "implementation/usb_init.h"
#include "gadget/f_hid.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

int test_usb_custom_init(int argc, char *argv[]);
int test_usb_custom_deinit(int argc, char *argv[]);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif