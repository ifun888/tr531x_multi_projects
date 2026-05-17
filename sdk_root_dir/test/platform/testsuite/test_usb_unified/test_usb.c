/**
 * Copyright (c) Triductor 2022-2023. All rights reserved. \n
 *
 * Description: Test usb source \n
 * Author: Triductor \n
 * History: \n
 * 2022-07-09, Create file. \n
 */
#include "chip_io.h"
#include "cmsis_os2.h"
#include "gadget/f_hid.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "securec.h"
#include "test_suite_errors.h"
#include "test_suite_log.h"
#include "test_suite.h"
#include "test_usb_custom.h"
#include "test_usb.h"

void add_usb_test_case(void)
{
    uapi_test_suite_add_function("usb_custom_init", "usage:usb_custom_init", test_usb_custom_init);
    uapi_test_suite_add_function("usb_custom_deinit", "usage:usb_custom_deinit", test_usb_custom_deinit);
}
