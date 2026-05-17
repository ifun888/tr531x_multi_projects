/**
 * Copyright (c) Triductor 2023-2023. All rights reserved. \n
 *
 * Description: SLE msg source \n
 * Author: Triductor \n
 * History: \n
 * 2023-04-03, Create file. \n
 */
#include "pinctrl.h"
#include "spi.h"
#include "osal_msgqueue.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"
#include "pinctrl.h"
#include "soc_osal.h"
#include "gpio.h"
#include "tcxo.h"
#include "msg.h"

#define MOUSE_QUEUE_DELAY         0xFFFFFFFF

static unsigned long sle_msgqueue_id;

void sle_create_msgqueue(void)
{
    if (osal_msg_queue_create("sle_msgqueue", SLE_MSG_QUEUE_LEN, \
        (unsigned long *)&sle_msgqueue_id, 0, SLE_MSG_QUEUE_MAX_SIZE) != OSAL_SUCCESS) {
        osal_printk("sle_create_msgqueue,message queue create failed!\n");
    }
    osal_printk("sle_create_msgqueue, message queue create  success!\r\n");
}

void sle_delete_msgqueue(void)
{
    osal_msg_queue_delete(sle_msgqueue_id);
}

void sle_write_msgqueue(uint8_t *buffer_addr, uint16_t buffer_size)
{
    osal_msg_queue_write_copy(sle_msgqueue_id, (void *)buffer_addr, \
                              (uint32_t)buffer_size, 0);
}

int32_t sle_receive_msgqueue(uint8_t *buffer_addr, uint32_t *buffer_size)
{
    return osal_msg_queue_read_copy(sle_msgqueue_id, (void *)buffer_addr, \
                                    buffer_size, MOUSE_QUEUE_DELAY);
}

void sle_rx_buf_init(uint8_t *buffer_addr, uint32_t *buffer_size)
{
    *buffer_size = SLE_MSG_QUEUE_MAX_SIZE;
    (void)memset_s(buffer_addr, *buffer_size, 0, *buffer_size);
}
