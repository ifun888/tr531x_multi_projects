/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: Provide USB Testsuite header \n
 * Author: Triductor \n
 * History: \n
 * 2023-03-31, Create file. \n
 */
#ifndef TEST_USB_H
#define TEST_USB_H

#include "implementation/usb_init.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * @defgroup testcase_usb USB
 * @ingroup  testcase
 * @{
 */

/**
 * @if Eng
 * @brief  Add the testcase of USB.
 * @else
 * @brief  添加USB测试用例
 * @endif
 */
void add_usb_test_case(void);

/**
 * @}
 */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif