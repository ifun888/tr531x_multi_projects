/**
 * Copyright (c) Triductor. 2022-2022. All rights reserved.
 *
 * Description: Provides sio hal source \n
 *
 * History: \n
 * 2022-08-01, Create file. \n
 */
#include <stdint.h>
#include "securec.h"
#include "tcxo.h"
#include "soc_osal.h"
#include "sio_porting.h"
#include "hal_sio_v150.h"

#define SIO_LOOP_INTMASK                0x3c
#define SIO_WRITE_INTMASK               0x3d
#define SIO_READ_INTMASK                0x3e
#define SIO_FIFO_SIZE                   16
#define SIO_I2S_RX_THRESHOLD            0xF
#define SIO_I2S_TX_THRESHOLD            0
#define SIO_ONCE_TRANSFER_LEN           (SIO_FIFO_SIZE - SIO_I2S_TX_THRESHOLD)

typedef struct hal_i2s_trans_inf {
    bool trans_succ;
    osal_semaphore trans_sem;
} hal_i2s_trans_inf_t;

static hal_i2s_trans_inf_t g_tx_trans[I2S_MAX_NUMBER] = { 0 };
static hal_sio_rx_data_t g_sio_read_data[I2S_MAX_NUMBER] = { 0 };
static hal_sio_callback_t g_hal_sio_callback[I2S_MAX_NUMBER] = { 0 };
static hal_sio_config_t g_hal_sio_config[I2S_MAX_NUMBER] = { 0 };
static uint32_t g_i2s_tx_length = 0;
static uint32_t g_i2s_tx_index = 0;
static uint32_t *g_write_left_data = NULL;
static uint32_t *g_write_right_data = NULL;

#pragma weak hal_sio_init = hal_sio_v150_init
errcode_t hal_sio_v150_init(sio_bus_t bus)
{
    if (hal_sio_regs_init(bus) != ERRCODE_SUCC) {
        return ERRCODE_SIO_REG_ADDR_INVALID;
    }
    g_i2s_tx_length = 0;
    g_i2s_tx_index = 0;
    sio_porting_register_irq(bus);
    osal_sem_init(&(g_tx_trans[bus].trans_sem), 0);
    return ERRCODE_SUCC;
}

#pragma weak hal_sio_deinit = hal_sio_v150_deinit
void hal_sio_v150_deinit(sio_bus_t bus)
{
    hal_sio_v150_set_rx_enable(bus, 0);
    hal_sio_v150_set_tx_enable(bus, 0);
    hal_sio_v150_ct_clr_set_rst_n(bus, 1);
    sio_porting_bclk_clock_enable(false);
    sio_porting_unregister_irq(bus);
    hal_sio_regs_deinit(bus);
    osal_sem_destroy(&(g_tx_trans[bus].trans_sem));
}

#pragma weak hal_sio_set_crg_clock_enable = hal_sio_v150_crg_clock_enable
void hal_sio_v150_crg_clock_enable(sio_bus_t bus, bool enable)
{
    unused(bus);
    unused(enable);
}

#pragma weak hal_sio_set_tx_enable = hal_sio_v150_set_tx_enable
void hal_sio_v150_set_tx_enable(sio_bus_t bus, uint32_t val)
{
    if (val == 0) {
        hal_sio_v150_ct_clr_set_tx_enable(bus, 1);
    } else {
        hal_sio_v150_ct_set_set_tx_enable(bus, 1);
    }
}

#pragma weak hal_sio_set_rx_enable = hal_sio_v150_set_rx_enable
void hal_sio_v150_set_rx_enable(sio_bus_t bus, uint32_t val)
{
    if (val == 0) {
        hal_sio_v150_ct_clr_set_rx_enable(bus, 1);
    } else {
        hal_sio_v150_ct_set_set_rx_enable(bus, 1);
    }
}

#pragma weak hal_sio_set_config = hal_sio_v150_set_config
void hal_sio_v150_set_config(sio_bus_t bus, const hal_sio_config_t *config)
{
    (void)memcpy_s(&g_hal_sio_config[bus], sizeof(hal_sio_config_t), config, sizeof(hal_sio_config_t));
}

#pragma weak hal_sio_get_config = hal_sio_v150_get_config
errcode_t hal_sio_v150_get_config(sio_bus_t bus, hal_sio_config_t *config)
{
    hal_sio_config_t *config_temp = (hal_sio_config_t *)config;
    if (config_temp == NULL) {
        return ERRCODE_INVALID_PARAM;
    }

    (void)memcpy_s(config_temp, sizeof(hal_sio_config_t), &g_hal_sio_config[bus], sizeof(hal_sio_config_t));

    return ERRCODE_SUCC;
}

static void hal_sio_v150_extend_receive_config(sio_bus_t bus, hal_sio_channel_number_t channels_num)
{
    hal_sio_v150_mode_set_ext_rec_en(bus, 1);
    hal_sio_v150_mode_set_chn_num(bus, channels_num);
}

static void hal_sio_v150_data_width_set(sio_bus_t bus, hal_sio_data_width_t data_width)
{
    hal_sio_v150_data_width_set_set_tx(bus, data_width);
    hal_sio_v150_data_width_set_set_rx(bus, data_width);
}

static void hal_i2s_config(sio_bus_t bus, hal_sio_mode_t mode)
{
    hal_sio_v150_set_intmask(bus, SIO_LOOP_INTMASK);
    hal_sio_v150_mode_set_mode(bus, mode);

    sio_porting_i2s_drive_mode(g_hal_sio_config[bus].drive_mode, g_hal_sio_config[bus].div_number,
                               g_hal_sio_config[bus].number_of_channels);
    sio_porting_i2s_gpio_mode(g_hal_sio_config[bus].drive_mode);
    hal_sio_v150_mode_set_clk_edge(bus, 1);

    if (g_hal_sio_config[bus].transfer_mode == MULTICHANNEL_MODE) {
        hal_sio_v150_extend_receive_config(bus, g_hal_sio_config[bus].channels_num);
    }

    hal_sio_v150_data_width_set(bus, g_hal_sio_config[bus].data_width);
    hal_sio_v150_start_pos_set_start_pos_write(bus, 0);
    hal_sio_v150_start_pos_set_start_pos_read(bus, 0);
    hal_sio_v150_pos_flag_set_start_pos_write(bus, 0);
    hal_sio_v150_pos_flag_set_start_pos_read(bus, 0);
    hal_sio_v150_ct_set_set_rst_n(bus, 1);
    hal_sio_v150_ct_set_set_intr_en(bus, 1);
    hal_sio_v150_ct_set_set_rx_fifo_threshold(bus, SIO_I2S_RX_THRESHOLD);
    hal_sio_v150_ct_set_set_tx_fifo_threshold(bus, SIO_I2S_TX_THRESHOLD);
}

static void hal_pcm_config(sio_bus_t bus, hal_sio_mode_t mode)
{
    sio_porting_register_irq(bus);
    hal_sio_v150_mode_set_mode(bus, mode);
    sio_porting_pcm_drive_mode(g_hal_sio_config[bus].drive_mode, g_hal_sio_config[bus].div_number,
                               g_hal_sio_config[bus].number_of_channels);
    hal_sio_v150_mode_set_pcm_mode(bus, g_hal_sio_config[bus].timing);

    if (g_hal_sio_config[bus].transfer_mode == MULTICHANNEL_MODE) {
        hal_sio_v150_extend_receive_config(bus, g_hal_sio_config[bus].channels_num);
        hal_sio_v150_mode_set_clk_edge(bus, g_hal_sio_config[bus].clk_edge);
    }

    hal_sio_v150_data_width_set(bus, g_hal_sio_config[bus].data_width);
    hal_sio_v150_ct_set_set_rst_n(bus, 1);
    hal_sio_v150_ct_set_set_intr_en(bus, 1);
    hal_sio_v150_ct_set_set_rx_fifo_threshold(bus, CONFIG_PCM_RX_THRESHOLD);
    hal_sio_v150_ct_set_set_tx_fifo_threshold(bus, CONFIG_PCM_TX_THRESHOLD);
}

#pragma weak hal_sio_read = hal_sio_v150_rx_enable
void hal_sio_v150_rx_enable(sio_bus_t bus, bool en)
{
    hal_i2s_config(bus, I2S_MODE);
    hal_sio_v150_set_intmask(bus, SIO_READ_INTMASK);
    hal_sio_v150_set_rx_enable(bus, (uint32_t)en);
}

static uint32_t hal_sio_read_data(sio_bus_t bus, hal_sio_voice_channel_t voice_channe)
{
    uint32_t data_temp = 0;
    if (voice_channe == SIO_LEFT) {
        data_temp = hal_sio_v150_left_rx_get_data(bus);
    } else {
        data_temp = hal_sio_v150_right_rx_get_data(bus);
    }
    return data_temp;
}

static void hal_sio_write_data(sio_bus_t bus, uint32_t data, hal_sio_voice_channel_t voice_channe)
{
    uint32_t data_temp = data;
    if (voice_channe == SIO_LEFT) {
        hal_sio_v150_left_tx_set_data(bus, data_temp);
    } else {
        hal_sio_v150_right_tx_set_data(bus, data_temp);
    }
}

static void hal_i2s_write(sio_bus_t bus, hal_sio_tx_data_t *data, hal_sio_mode_t mode)
{
    hal_i2s_config(bus, mode);
    hal_sio_v150_set_intmask(bus, SIO_WRITE_INTMASK);
    uint32_t sio_tx_num = 0;
    g_i2s_tx_length = data->length;
    sio_tx_num = g_i2s_tx_length < SIO_FIFO_SIZE ? g_i2s_tx_length : SIO_FIFO_SIZE;
    g_write_left_data = data->left_buff;
    g_write_right_data = data->right_buff;

    for (uint32_t i = 0; i < sio_tx_num; i++) {
        hal_sio_write_data(bus, g_write_left_data[i], SIO_LEFT);
        hal_sio_write_data(bus, g_write_right_data[i], SIO_RIGHT);
    }
    g_i2s_tx_index += sio_tx_num;

    sio_porting_bclk_clock_enable(true);
    hal_sio_v150_set_tx_enable(bus, 1);

    if (osal_sem_down(&(g_tx_trans[bus].trans_sem)) != OSAL_SUCCESS) {
        return;
    }
    if (!g_tx_trans[bus].trans_succ) {
        return;
    }

    hal_sio_v150_set_tx_enable(bus, 0);
    sio_porting_bclk_clock_enable(false);
    g_tx_trans[bus].trans_succ = false;
}

static void hal_pcm_write(sio_bus_t bus, hal_sio_tx_data_t *data, hal_sio_mode_t mode)
{
    hal_pcm_config(bus, mode);
    uint32_t *temp_tx_right_data = data->right_buff;
    uint32_t trans_time = data->length;
    hal_sio_v150_set_tx_enable(bus, true);
    for (uint32_t i = 0; i < trans_time; i++) {
        hal_sio_write_data(bus, temp_tx_right_data[i], SIO_RIGHT);
    }
    hal_sio_v150_set_tx_enable(bus, false);
}

#pragma weak hal_sio_write = hal_sio_v150_write
void hal_sio_v150_write(sio_bus_t bus, hal_sio_tx_data_t *data, hal_sio_mode_t mode)
{
    if (mode == I2S_MODE) {
        hal_i2s_write(bus, data, mode);
    } else {
        hal_pcm_write(bus, data, mode);
    }
}

#pragma weak hal_sio_register_callback = hal_sio_v150_register
void hal_sio_v150_register(sio_bus_t bus, hal_sio_callback_t callback)
{
    g_hal_sio_callback[bus] = callback;
}

#pragma weak hal_sio_unregister_callback = hal_sio_v150_unregister
void hal_sio_v150_unregister(sio_bus_t bus)
{
    g_hal_sio_callback[bus] = NULL;
}

#pragma weak hal_sio_get_data = hal_sio_v150_get_data
void hal_sio_v150_get_data(sio_bus_t bus, hal_sio_rx_data_t *data)
{
    *data = g_sio_read_data[bus];
}

void hal_sio_v150_irq_handler(sio_bus_t bus)
{
    uint32_t rx_int_status = hal_sio_v150_intstatus_get_rx_intr(bus);
    uint32_t tx_int_status = hal_sio_v150_intstatus_get_tx_intr(bus);
    if (rx_int_status != 0) {
        uint32_t trans_frames = hal_sio_v150_rx_sta_get_rx_right_depth(bus);
        while (trans_frames-- > 0) {
            g_sio_read_data[bus].left_buff[g_sio_read_data[bus].length] = hal_sio_read_data(bus, SIO_LEFT);
            g_sio_read_data[bus].right_buff[g_sio_read_data[bus].length] = hal_sio_read_data(bus, SIO_RIGHT);
            g_sio_read_data[bus].length++;
        }

        if (g_hal_sio_callback[bus]) {
            g_hal_sio_callback[bus](g_sio_read_data[bus].left_buff, g_sio_read_data[bus].right_buff,
                                    g_sio_read_data[bus].length);
            (void)memset_s(((void *)&g_sio_read_data), sizeof(hal_sio_rx_data_t), 0, sizeof(hal_sio_rx_data_t));
            g_sio_read_data[bus].length = 0;
        }
        hal_sio_v150_intclr_set_rx_intr(bus, 1);
    }

    if (tx_int_status != 0) {
        if (g_i2s_tx_index == 0 && g_i2s_tx_length == 0) {
            hal_sio_v150_intclr_set_tx_intr(bus, 1);
        } else if (g_i2s_tx_index < g_i2s_tx_length) {
            uint32_t write_len = g_i2s_tx_length - g_i2s_tx_index;
            write_len = write_len < SIO_ONCE_TRANSFER_LEN ? write_len : SIO_ONCE_TRANSFER_LEN;
            for (uint32_t i = 0; i < write_len; i++) {
                hal_sio_write_data(bus, g_write_left_data[g_i2s_tx_index + i], SIO_LEFT);
                hal_sio_write_data(bus, g_write_right_data[g_i2s_tx_index + i], SIO_RIGHT);
            }
            g_i2s_tx_index += write_len;
            hal_sio_v150_intclr_set_tx_intr(bus, 1);
        } else {
            hal_sio_v150_intclr_set_tx_intr(bus, 1);
            hal_sio_v150_set_intmask(bus, SIO_READ_INTMASK);
            g_i2s_tx_index = 0;
            g_i2s_tx_length = 0;
            g_tx_trans[bus].trans_succ = true;
            osal_sem_up(&(g_tx_trans[bus].trans_sem));
        }
    }
}

#if defined(CONFIG_I2S_SUPPORT_LOOPBACK)
#pragma weak hal_sio_set_loop_mode = hal_sio_v150_loop
void hal_sio_v150_loop(sio_bus_t bus, bool en)
{
    hal_sio_v150_version_set_loop(bus, (uint32_t)en);
}

#pragma weak hal_sio_start_loop_trans = hal_sio_v150_loop_trans
void hal_sio_v150_loop_trans(sio_bus_t bus, hal_sio_tx_data_t *data, hal_sio_mode_t mode)
{
    hal_i2s_config(bus, mode);
    uint32_t sio_tx_num = 0;
    g_i2s_tx_length = data->length;
    sio_tx_num = g_i2s_tx_length < SIO_FIFO_SIZE ? g_i2s_tx_length : SIO_FIFO_SIZE;
    g_write_left_data = data->left_buff;
    g_write_right_data = data->right_buff;

    for (uint32_t i = 0; i < sio_tx_num; i++) {
        hal_sio_write_data(bus, g_write_left_data[i], SIO_LEFT);
        hal_sio_write_data(bus, g_write_right_data[i], SIO_RIGHT);
    }
    g_i2s_tx_index += sio_tx_num;

    sio_porting_bclk_clock_enable(true);
    hal_sio_v150_set_rx_enable(bus, 1);
    hal_sio_v150_set_tx_enable(bus, 1);

    if (osal_sem_down(&(g_tx_trans[bus].trans_sem)) != OSAL_SUCCESS) {
        return;
    }
    if (!g_tx_trans[bus].trans_succ) {
        return;
    }

    hal_sio_v150_set_tx_enable(bus, 0);
    hal_sio_v150_set_rx_enable(bus, 0);
    sio_porting_bclk_clock_enable(false);
    g_tx_trans[bus].trans_succ = false;
}
#endif

#if defined(CONFIG_I2S_SUPPORT_DMA)
#define SIO_INTMASK_TX_DISABLED     0x32
#define SIO_INTMASK_RX_DISABLED     0xd

#pragma weak hal_sio_set_dma_config = hal_sio_v150_dma_cfg
void hal_sio_v150_dma_cfg(sio_bus_t bus, hal_i2s_dma_attr_t *attr)
{
    hal_i2s_config(bus, I2S_MODE);
    hal_sio_v150_pos_merge_set_en(bus, 1);
    if (attr->tx_dma_enable) {
        hal_sio_v150_ct_set_set_tx_fifo_threshold(bus, attr->tx_int_threshold);
        uint32_t mask = sios_v150_regs(bus)->intmask;
        mask = (mask | SIO_INTMASK_TX_DISABLED);
        hal_sio_v150_set_intmask(bus, mask);
    }
    if (attr->rx_dma_enable) {
        hal_sio_v150_ct_set_set_rx_fifo_threshold(bus, attr->rx_int_threshold);
        uint32_t mask = sios_v150_regs(bus)->intmask;
        mask = (mask | SIO_INTMASK_RX_DISABLED);
        hal_sio_v150_set_intmask(bus, mask);
    }
}
#endif
