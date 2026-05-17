/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: USB Initialize Source. \n
 * Author: Triductor \n
 * History: \n
 * 2023-07-07, Create file. \n
 */
#include <stdbool.h>
#include "gadget/f_hid.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "common_def.h"
#include "cmsis_os2.h"
#include "test_usb_custom.h"

#define USB_RECV_STACK_SIZE         0x400
#define RECV_MAX_LENGTH             64
#define USB_RECV_FAIL_DELAY         10
#define DELAY_FOR_USB_DETECTED      200

static bool g_usb_inited = false;
static osal_task *g_usb_recv_task = NULL;

extern ssize_t ftest_recv_data(char *buf, size_t buflen);
extern ssize_t ftest_send_data(const char *buf, size_t buflen, uint8_t in_endpoint_index);
static int usb_recv_task(void *para)
{
    unused(para);
    osDelay(DELAY_FOR_USB_DETECTED);
    uint8_t recv_data[RECV_MAX_LENGTH] = { 0 };
    uint8_t tx_data[RECV_MAX_LENGTH] = { "usb_custom_tx_test" };
    for (;;) {
        /*recv from pc */
        ssize_t ret = ftest_recv_data((char*)recv_data, RECV_MAX_LENGTH);
        if (ret <= 0) {
            if(ret == -2) {
                break;
            }
            osDelay(USB_RECV_FAIL_DELAY);
            continue;
        }
        recv_data[ret] = '\0';
        uint8_t *p_data = recv_data;
        osal_printk("%s\r\n", p_data);
        /*send to pc for test */
        ftest_send_data((const char *)tx_data, sizeof(tx_data), 0);
    }
    return 0;
}

static int usb_custom_init_app(device_type dtype)
{
    if (g_usb_inited == true) {
        return -1;
    }

    if (usb_init(DEVICE, dtype) != 0) {
        return -1;
    }
    g_usb_recv_task = osal_kthread_create(usb_recv_task, NULL, "usb_custom_task", USB_RECV_STACK_SIZE);
    g_usb_inited = true;
    return 0;
}

static int usb_custom_deinit_app(void)
{
    if (g_usb_inited == false) {
        return 0;
    }
    (void)usb_deinit();
    g_usb_inited = false;
    return 0;
}

int test_usb_custom_init(int argc, char *argv[])
{
    unused(argc);
    unused(argv);
    usb_custom_init_app(DEV_CUSTOM);
    return 0;
}

int test_usb_custom_deinit(int argc, char *argv[])
{
    unused(argc);
    unused(argv);
    usb_custom_deinit_app();
    return 0;
}
