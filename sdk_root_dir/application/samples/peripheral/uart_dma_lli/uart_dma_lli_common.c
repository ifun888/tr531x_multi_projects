/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: UART DMA LLI Common Interface Source. \n
 *
 * History: \n
 * 2024-06-13, Create file. \n
 */
#include "uart_dma_lli_common.h"

uint8_t g_app_uart_header_buff[UART_MSG_HEADER_LEN] = { 0 };
uint8_t g_app_uart_payload_buff[CONFIG_UART_MSG_MAX_LEN_BY_DMA_LLI] = { 0 };
uint8_t g_app_uart_tail_buff[UART_MSG_TAIL_LEN] = { 0 };

volatile uint16_t g_app_uart_expect_rx_len = 0;

dma_channel_t g_app_uart_dma_channel = 0;
dma_ch_user_peripheral_config_t g_app_uart_user_cfg = { 0 };

static uart_buffer_config_t g_app_uart_buffer_config = {
    .rx_buffer = g_app_uart_header_buff,
    .rx_buffer_size = UART_MSG_HEADER_LEN
};

void app_uart_init_config(void)
{
    errcode_t ret = ERRCODE_FAIL;

    uart_attr_t attr = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_BIT_8,
        .stop_bits = UART_STOP_BIT_1,
        .parity = UART_PARITY_NONE
    };

    uart_extra_attr_t extra_attr = {
        .tx_dma_enable = true,
        .tx_int_threshold = UART_FIFO_INT_TX_LEVEL_EQ_0_CHARACTER,
        .rx_dma_enable = true,
        .rx_int_threshold = UART_FIFO_INT_RX_LEVEL_1_CHARACTER
    };

    uapi_dma_deinit();
    uapi_dma_init();
    uapi_dma_open();
    uapi_uart_deinit(CONFIG_UART_BUS_ID);
    ret = uapi_uart_init(CONFIG_UART_BUS_ID, NULL, &attr, &extra_attr, &g_app_uart_buffer_config);
    if (ret != ERRCODE_SUCC) {
        osal_printk("uart init error, ret= 0x%x!\n", ret);
        return;
    }
}

void app_uart_dma_tx_transfer_config(uart_bus_t bus, dma_channel_t *dma_channel,
                                     dma_ch_user_peripheral_config_t *user_cfg)
{
    *dma_channel = (dma_channel_t)uapi_dma_get_lli_channel(HAL_DMA_BURST_TRANSACTION_LENGTH_1,
                                                           uart_port_get_dma_trans_dest_handshaking(bus));

    user_cfg->src = (uint32_t)(uintptr_t)g_app_uart_header_buff;
    user_cfg->dest = uart_porting_base_addr_get(bus) + UART_DATA_FIFO_OFFSET_ADDR;
    user_cfg->src_handshaking = 0;
    user_cfg->dest_handshaking = uart_port_get_dma_trans_dest_handshaking(bus);
    user_cfg->src_width = HAL_DMA_TRANSFER_WIDTH_8;
    user_cfg->dest_width = HAL_DMA_TRANSFER_WIDTH_8;
    user_cfg->transfer_num = (uint16_t)(UART_MSG_HEADER_LEN >> user_cfg->src_width);
    user_cfg->trans_type = HAL_DMA_TRANS_MEMORY_TO_PERIPHERAL_DMA;
    user_cfg->trans_dir = HAL_DMA_TRANSFER_DIR_MEM_TO_PERIPHERAL;
    user_cfg->priority = HAL_DMA_CH_PRIORITY_0;
    user_cfg->burst_length = HAL_DMA_BURST_TRANSACTION_LENGTH_1;
    user_cfg->src_increment = HAL_DMA_ADDRESS_INC_INCREMENT;
    user_cfg->dest_increment = HAL_DMA_ADDRESS_INC_NO_CHANGE;
    user_cfg->protection = HAL_DMA_PROTECTION_CONTROL_NONE;
}

void app_uart_dma_rx_transfer_config(uart_bus_t bus, dma_channel_t *dma_channel,
                                     dma_ch_user_peripheral_config_t *user_cfg)
{
    *dma_channel = (dma_channel_t)uapi_dma_get_lli_channel(HAL_DMA_BURST_TRANSACTION_LENGTH_1,
                                                           uart_port_get_dma_trans_src_handshaking(bus));

    user_cfg->src = uart_porting_base_addr_get(bus) + UART_DATA_FIFO_OFFSET_ADDR;
    user_cfg->dest = (uint32_t)(uintptr_t)g_app_uart_header_buff;
    user_cfg->src_handshaking = uart_port_get_dma_trans_src_handshaking(bus);
    user_cfg->dest_handshaking = 0;
    user_cfg->src_width = HAL_DMA_TRANSFER_WIDTH_8;
    user_cfg->dest_width = HAL_DMA_TRANSFER_WIDTH_8;
    user_cfg->transfer_num = (uint16_t)(UART_MSG_HEADER_LEN >> user_cfg->src_width);
    user_cfg->trans_type = HAL_DMA_TRANS_PERIPHERAL_TO_MEMORY_DMA;
    user_cfg->trans_dir = HAL_DMA_TRANSFER_DIR_PERIPHERAL_TO_MEM;
    user_cfg->priority = HAL_DMA_CH_PRIORITY_0;
    user_cfg->burst_length = HAL_DMA_BURST_TRANSACTION_LENGTH_1;
    user_cfg->src_increment = HAL_DMA_ADDRESS_INC_NO_CHANGE;
    user_cfg->dest_increment = HAL_DMA_ADDRESS_INC_INCREMENT;
    user_cfg->protection = HAL_DMA_PROTECTION_CONTROL_NONE;
}

void app_uart_dma_single_node_transfer(dma_transfer_cb_t cb)
{
    errcode_t ret = ERRCODE_FAIL;

    ret = uapi_dma_configure_peripheral_transfer_lli(g_app_uart_dma_channel, &g_app_uart_user_cfg, cb);
    if (ret != ERRCODE_SUCC) {
        osal_printk("uart dma config peripheral transfer lli fail, ret:%x!\r\n", ret);
    }

    ret = uapi_dma_enable_lli(g_app_uart_dma_channel, cb, (uintptr_t)NULL);
    if (ret != ERRCODE_SUCC) {
        osal_printk("uart dma enable lli transfer fail, ret:%x!\r\n", ret);
    }
}

void app_uart_dma_all_node_transfer(dma_transfer_cb_t cb, uint16_t payload_buff_len)
{
    errcode_t ret = ERRCODE_FAIL;
    /* Uart dma config. */
    app_uart_dma_tx_transfer_config(CONFIG_UART_BUS_ID, &g_app_uart_dma_channel, &g_app_uart_user_cfg);
    /* Add the message header, message content, and message tail to the DMA linked list. */
    ret = uapi_dma_configure_peripheral_transfer_lli(g_app_uart_dma_channel, &g_app_uart_user_cfg,
                                                     cb);
    if (ret != ERRCODE_SUCC) {
        osal_printk("uart dma config header node peripheral transfer lli fail, ret:%x!\r\n", ret);
    }

    g_app_uart_user_cfg.src = (uint32_t)(uintptr_t)g_app_uart_payload_buff;
    g_app_uart_user_cfg.transfer_num = (uint16_t)(payload_buff_len >> g_app_uart_user_cfg.src_width);
    ret = uapi_dma_configure_peripheral_transfer_lli(g_app_uart_dma_channel, &g_app_uart_user_cfg,
                                                     cb);
    if (ret != ERRCODE_SUCC) {
        osal_printk("uart dma config payload node peripheral transfer lli fail, ret:%x!\r\n", ret);
    }
    g_app_uart_user_cfg.src = (uint32_t)(uintptr_t)g_app_uart_tail_buff;
    g_app_uart_user_cfg.transfer_num = (uint16_t)(UART_MSG_TAIL_LEN >> g_app_uart_user_cfg.src_width);
    ret = uapi_dma_configure_peripheral_transfer_lli(g_app_uart_dma_channel, &g_app_uart_user_cfg,
                                                     cb);
    if (ret != ERRCODE_SUCC) {
        osal_printk("uart dma config tail node peripheral transfer lli fail, ret:%x!\r\n", ret);
    }

    /* Enable uart dma linked list. */
    ret = uapi_dma_enable_lli(g_app_uart_dma_channel, cb, (uintptr_t)NULL);
    if (ret != ERRCODE_SUCC) {
        osal_printk("uart dma enable lli memory transfer fail, ret:%x!\r\n", ret);
    }
}