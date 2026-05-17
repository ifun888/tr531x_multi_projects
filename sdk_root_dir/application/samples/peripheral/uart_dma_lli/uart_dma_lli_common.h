/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: UART DMA LLI Common Interface header. \n
 *
 * History: \n
 * 2024-06-13, Create file. \n
 */
#ifndef UART_DMA_LLI_H
#define UART_DMA_LLI_H

#include "pinctrl.h"
#include "uart.h"
#include "watchdog.h"
#include "dma.h"
#include "hal_dma.h"
#include "soc_osal.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define UART_BAUDRATE                      115200
#define UART_DATA_FIFO_OFFSET_ADDR         0x4

/* Uart message start byte. */
#define UART_MSG_START                     0x7E
/* Uart message end byte. */
#define UART_MSG_TAIL                      0x7E

/* Uart message header length. */
#define UART_MSG_HEADER_LEN                4
/* Uart message packet tail length. */
#define UART_MSG_TAIL_LEN                  1

enum uart_data_status_enum {
    MSG_HEADER,
    MSG_PAYLOAD,
    MSG_TAIL
};

typedef struct {
    uint8_t uart_msg_start;
    uint8_t uart_msg_type;
    uint16_t uart_msg_len;
} uart_msg_header_t;

extern uint8_t g_app_uart_header_buff[UART_MSG_HEADER_LEN];
extern uint8_t g_app_uart_payload_buff[CONFIG_UART_MSG_MAX_LEN_BY_DMA_LLI];
extern uint8_t g_app_uart_tail_buff[UART_MSG_TAIL_LEN];
extern volatile uint16_t g_app_uart_expect_rx_len;
extern dma_channel_t g_app_uart_dma_channel;
extern dma_ch_user_peripheral_config_t g_app_uart_user_cfg;

void app_uart_init_config(void);

void app_uart_dma_tx_transfer_config(uart_bus_t bus, dma_channel_t *dma_channel,
                                     dma_ch_user_peripheral_config_t *user_cfg);

void app_uart_dma_rx_transfer_config(uart_bus_t bus, dma_channel_t *dma_channel,
                                     dma_ch_user_peripheral_config_t *user_cfg);

void app_uart_dma_single_node_transfer(dma_transfer_cb_t cb);

void app_uart_dma_all_node_transfer(dma_transfer_cb_t cb, uint16_t payload_buff_len);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif