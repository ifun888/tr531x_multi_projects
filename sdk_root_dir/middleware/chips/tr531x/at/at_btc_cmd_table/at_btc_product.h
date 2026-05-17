/**
 * Copyright (c) Triductor. 2023-2023. All rights reserved.
 *
 * Description: At bt header \n
 *
 */

#ifndef AT_BTC_PRODUCT_H
#define AT_BTC_PRODUCT_H

#include "td_base.h"
#include "at.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef XO_32M_CALI
at_ret_t xo_ctrim_cali_cmd(const xo_ctrim_cali_param_args_t *args);
at_ret_t xo_ctrim_cali_write_efuse_cmd(void);
at_ret_t xo_ctrim_cali_read_efuse_cmd(void);
at_ret_t xo_ctrim_get_reg_val_cmd(void);
at_ret_t xo_ctrim_cali_write_flash_cmd(void);
at_ret_t xo_ctrim_cali_read_flash_cmd(void);
#endif
at_ret_t bt_at_ble_rf_tx_cmd(const ble_rf_tx_param_args_t *args);
at_ret_t bt_at_ble_rf_rx_cmd(const ble_rf_rx_param_args_t *args);
at_ret_t bt_at_ble_rf_trxend_cmd(void);
at_ret_t bt_at_ble_reset_cmd(void);
at_ret_t bt_at_sle_rf_tx_cmd(const sle_rf_tx_param_args_t *args);
at_ret_t bt_at_sle_rf_rx_cmd(const sle_rf_rx_param_args_t *args);
at_ret_t bt_at_sle_rf_trxend_cmd(void);
at_ret_t bt_at_sle_reset_cmd(void);
at_ret_t bt_at_cfo_rpt_cmd(void);
at_ret_t bt_at_cfo_rpt_rssi_limit_cmd(const rf_sle_cfo_rpt_rssi_limit_t *args);
at_ret_t bt_at_rf_cali_nv_cmd(void);
at_ret_t bt_at_read_cali_nv_cmd(void);
at_ret_t bt_at_write_cali_nv_cmd(void);
at_ret_t bt_at_customize_nv_cmd(const bt_write_customize_nv_param_args_t *args);
at_ret_t bt_at_read_customize_nv_cmd(void);
at_ret_t bt_at_rf_single_tone_cmd(const rf_single_tone_param_args_t *args);
at_ret_t bt_at_enable_sle_cmd(void);
at_ret_t bt_at_enable_ble_cmd(void);
at_ret_t bt_at_ble_register_callback_cmd(void);
at_ret_t bt_at_sle_register_callback_cmd(void);
at_ret_t bt_at_read_dieid_cmd(void);
at_ret_t bt_at_set_fem_enable_flag(const fem_switch_param_args_t *args);
at_ret_t bt_at_set_addr_cmd(const bt_write_addr_flash_param_args_t *args);
at_ret_t bt_at_stop_sle_adv_cmd(void);
at_ret_t bt_at_stop_ble_adv_cmd(void);
at_ret_t bt_at_stop_sle_seek_cmd(void);
at_ret_t bt_at_stop_ble_scan_cmd(void);
at_ret_t bt_at_stay_work_state(void);
#if defined(CONFIG_SAMPLE_SUPPORT_SLE_KEYBOARD) || defined(CONFIG_SAMPLE_SUPPORT_SLE_RCU_SERVER)
at_ret_t bt_at_send_key_value_cmd(const bt_at_send_key_value_param_args_t *args);
#endif

void keyscan_info_report_at_cmd(int key_nums, uint8_t *key_values);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
void at_customize_features_set(uint8_t *features, uint8_t offset);
#endif /* end of at_btc_product.h */
