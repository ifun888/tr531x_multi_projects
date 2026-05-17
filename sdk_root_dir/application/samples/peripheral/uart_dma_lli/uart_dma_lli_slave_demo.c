/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: UART DMA LLI Slave Sample Source. \n
 *
 * History: \n
 * 2024-06-13, Create file. \n
 */
#include "app_init.h"
#include "uart_dma_lli_common.h"

#define UART_RX_DELAY_MS                   2000
#define UART_DMA_RX_TRANSFER_EVENT         1
#define UART_DMA_TX_TRANSFER_EVENT         2

#define UART_MSG_HEADER_BUFF_INDEX2        2
#define UART_MSG_HEADER_BUFF_INDEX3        3

#define UART_MSG_MIN_LEN                   6
#define UART_MSG_TYPE_MAX                  0x39

#define UART_TASK_PRIO                     24
#define UART_TASK_DURATION_MS              500
#define UART_TASK_STACK_SIZE               0x1000

static volatile uint8_t  g_app_uart_msg_type = 0;
static volatile uint16_t g_app_uart_msg_len = 0;
static volatile uint8_t g_app_uart_rx_status = MSG_HEADER;
static osal_event g_app_uart_dma_id;

static void uart_link_dma_rx_callback_func(uint8_t intr, uint8_t channel, uintptr_t arg);

/* Uart_wrong_status_handle. */
static void uart_wrong_status_handle(void)
{
    errcode_t ret = ERRCODE_FAIL;
    osal_printk("uart_wrong_status_handle enter!\r\n");

    /* When the header is judged incorrectly, print the 4-byte content of the header. */
    if (g_app_uart_rx_status == MSG_HEADER) {
        osal_printk("Header rcv error:[0x%x 0x%x 0x%x 0x%x]\r\n",
            g_app_uart_header_buff[0], g_app_uart_header_buff[1], g_app_uart_header_buff[UART_MSG_HEADER_BUFF_INDEX2],
            g_app_uart_header_buff[UART_MSG_HEADER_BUFF_INDEX3]);
        ret = uapi_dma_end_transfer(g_app_uart_dma_channel);
        if (ret != ERRCODE_SUCC) {
            osal_printk("uart rx dma end header transfer fail, ret:%x!\r\n", ret);
        }
        app_uart_dma_single_node_transfer(uart_link_dma_rx_callback_func);
        return;
    }
    ret = uapi_dma_end_transfer(g_app_uart_dma_channel);
    if (ret != ERRCODE_SUCC) {
        osal_printk("uart rx dma end transfer fail, ret:%x!\r\n", ret);
    }
    g_app_uart_rx_status = MSG_HEADER;
}

/* Uart_msg_header_handle. */
static void uart_msg_header_handle(void)
{
    uart_msg_header_t *msg_header = (uart_msg_header_t *)g_app_uart_header_buff;

    for (uint8_t i = 0; i < UART_MSG_HEADER_LEN; i++) {
        osal_printk("header_buff[%d]:0x%x\r\n", i, g_app_uart_header_buff[i]);
    }

    /* Determine whether the first byte of the packet header is 0x7E. */
    if (unlikely(msg_header->uart_msg_start != UART_MSG_START)) {
        uart_wrong_status_handle();
        return;
    }

    /*  Determine whether the packet type and packet length are incorrect. */
    if (unlikely(msg_header->uart_msg_type >= UART_MSG_TYPE_MAX)) {
        uart_wrong_status_handle();
        return;
    }

    if (unlikely((msg_header->uart_msg_len < UART_MSG_MIN_LEN) ||
        (msg_header->uart_msg_len > CONFIG_UART_MSG_MAX_LEN_BY_DMA_LLI))) {
        uart_wrong_status_handle();
        return;
    }

    /* Get the type and length of the package. */
    g_app_uart_msg_type = msg_header->uart_msg_type;
    g_app_uart_msg_len = msg_header->uart_msg_len;

    /* Default data length and storage address to be received. */
    g_app_uart_expect_rx_len = g_app_uart_msg_len - (UART_MSG_HEADER_LEN + UART_MSG_TAIL_LEN);

    errcode_t ret = uapi_dma_end_transfer(g_app_uart_dma_channel);
    if (ret != ERRCODE_SUCC) {
        osal_printk("uart rx dma end header transfer fail, ret:%x!\r\n", ret);
    }

    g_app_uart_rx_status = MSG_PAYLOAD;
    g_app_uart_user_cfg.dest = (uint32_t)(uintptr_t)g_app_uart_payload_buff;
    g_app_uart_user_cfg.transfer_num = (uint16_t)(g_app_uart_expect_rx_len >> g_app_uart_user_cfg.src_width);
    app_uart_dma_single_node_transfer(uart_link_dma_rx_callback_func);
}

/* Uart_msg_payload_handle. */
static void uart_msg_payload_handle(void)
{
    for (uint8_t i = 0; i < g_app_uart_expect_rx_len; i++) {
        osal_printk("payload_buff[%d]:0x%x\r\n", i, g_app_uart_payload_buff[i]);
    }
    errcode_t ret = uapi_dma_end_transfer(g_app_uart_dma_channel);
    if (ret != ERRCODE_SUCC) {
        osal_printk("uart rx dma end payload transfer fail, ret:%x!\r\n", ret);
    }
    g_app_uart_rx_status = MSG_TAIL;
    g_app_uart_user_cfg.dest = (uint32_t)(uintptr_t)g_app_uart_tail_buff;
    g_app_uart_user_cfg.transfer_num = (uint16_t)(UART_MSG_TAIL_LEN >> g_app_uart_user_cfg.src_width);
    app_uart_dma_single_node_transfer(uart_link_dma_rx_callback_func);
}

/* Uart_msg_tail_handle. */
static void uart_msg_tail_handle(void)
{
    osal_printk("tail_buff:0x%x\r\n", g_app_uart_tail_buff[0]);
    /* The end of the packet is not 0x7E. */
    if (unlikely(g_app_uart_tail_buff[0] != UART_MSG_TAIL)) {
        osal_printk("uart msg tail 0x%x is error!\r\n", g_app_uart_tail_buff[0]);
        uart_wrong_status_handle();
        g_app_uart_rx_status = MSG_TAIL;
        app_uart_dma_single_node_transfer(uart_link_dma_rx_callback_func);
        return;
    }

    /* Restore to initial state. */
    g_app_uart_rx_status = MSG_HEADER;
    g_app_uart_msg_type = 0;
    g_app_uart_msg_len = 0;
    if (osal_event_write(&g_app_uart_dma_id, UART_DMA_RX_TRANSFER_EVENT) != OSAL_SUCCESS) {
        osal_printk("osal_event_write rx fail!\r\n");
        return;
    }
}

/* Uart_link_dma_rx_callback_func. */
static void uart_link_dma_rx_callback_func(uint8_t intr, uint8_t channel, uintptr_t arg)
{
    unused(arg);
    switch (intr) {
        case HAL_DMA_INTERRUPT_TFR:
            switch (g_app_uart_rx_status) {
                case MSG_HEADER:
                    osal_printk("-------RX_MSG_HEADER------\r\n");
                    uart_msg_header_handle();
                    break;
                case MSG_PAYLOAD:
                    osal_printk("-------RX_MSG_PAYLOAD------\r\n");
                    uart_msg_payload_handle();
                    break;
                case MSG_TAIL:
                    osal_printk("-------RX_MSG_TAIL------\r\n");
                    uart_msg_tail_handle();
                    break;
                default:
                    osal_printk("-------wrong------\r\n");
                    uart_wrong_status_handle();
                    break;
            }
            break;
        case HAL_DMA_INTERRUPT_ERR:
            osal_printk("dma channel[%d] trans error\r\n", channel);
            break;
        default:
            break;
    }
}

/* Uart_link_dma_tx_callback_func. */
static void uart_link_dma_tx_callback_func(uint8_t intr, uint8_t channel, uintptr_t arg)
{
    unused(arg);
    switch (intr) {
        case HAL_DMA_INTERRUPT_TFR:
            if (osal_event_write(&g_app_uart_dma_id, UART_DMA_TX_TRANSFER_EVENT) != OSAL_SUCCESS) {
                osal_printk("osal_event_tx_write fail!\r\n");
                return;
            }
            break;
        case HAL_DMA_INTERRUPT_ERR:
            osal_printk("dma channel[%d] trans error\r\n", channel);
            break;
        default:
            break;
    }
}

static void *uart_dma_lli_slave_task(const char *arg)
{
    unused(arg);
    errcode_t ret = ERRCODE_FAIL;
    if (osal_event_init(&g_app_uart_dma_id) != OSAL_SUCCESS) {
        osal_printk("osal_event_init fail!\r\n");
        return NULL;
    }

    /* Uart init config. */
    app_uart_init_config();

    /* Let uart rx after tx is enabled, ensure that rx does not receive 0 at the beginning. */
    osal_msleep(UART_RX_DELAY_MS);
    while (1) {
        /* The delay here only serves as a buffer between sending and receiving. */
        osal_msleep(UART_TASK_DURATION_MS);
        /* Uart rx dma header node config. */
        app_uart_dma_rx_transfer_config(CONFIG_UART_BUS_ID, &g_app_uart_dma_channel, &g_app_uart_user_cfg);
        /* Uart rx dma header node transfer. */
        app_uart_dma_single_node_transfer(uart_link_dma_rx_callback_func);
        if (!(osal_event_read(&g_app_uart_dma_id, UART_DMA_RX_TRANSFER_EVENT, OSAL_WAIT_FOREVER,
                              OSAL_WAITMODE_AND | OSAL_WAITMODE_CLR))) {
            uapi_watchdog_kick();
            uapi_dma_end_transfer(g_app_uart_dma_channel);
            continue;
        }

        /* End the uart dma tail node channel transmission and release the tail linked list node configuration. */
        ret = uapi_dma_end_transfer(g_app_uart_dma_channel);
        if (ret != ERRCODE_SUCC) {
            osal_printk("uart rx dma end transfer fail, ret:%x!\r\n", ret);
        }

        app_uart_dma_all_node_transfer(uart_link_dma_tx_callback_func, g_app_uart_expect_rx_len);
        if (!(osal_event_read(&g_app_uart_dma_id, UART_DMA_TX_TRANSFER_EVENT, OSAL_WAIT_FOREVER,
                              OSAL_WAITMODE_AND | OSAL_WAITMODE_CLR))) {
            uapi_watchdog_kick();
            uapi_dma_end_transfer(g_app_uart_dma_channel);
            continue;
        }
        /* End the uart dma tx channel transmission and release all linked list node configuration. */
        ret = uapi_dma_end_transfer(g_app_uart_dma_channel);
        if (ret != ERRCODE_SUCC) {
            osal_printk("uart tx dma end transfer fail, ret:%x!\r\n", ret);
        }
    }

    return NULL;
}

static void uart_dma_lli_slave_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)uart_dma_lli_slave_task, 0, "UartDmaLLISlaveTask",
                                      UART_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, UART_TASK_PRIO);
    }
    osal_kthread_unlock();
}

/* Run the uart_dma_lli_slave_entry. */
app_run(uart_dma_lli_slave_entry);