#!/usr/bin/env python3tr5310
# encoding=utf-8
# ============================================================================
# @brief    Target Definitions File
# Copyright Triductor 2022-2022. All rights reserved.
# ============================================================================
tr5310_s_libs = {
    'tr5310-s-sle-peripheral': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5310-s-libgen',
        'defines': ['SUPPORT_SLE_PERIPHERAL', 'CONFIG_BG_SLE_ONLY']
    },
    'tr5310-s-sle-ble-peripheral': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5310-s-libgen',
        'defines': ['SUPPORT_SLE_BLE_PERIPHERAL']
    },
    'tr5310-s-sle-central': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5310-s-libgen',
        'defines': ['SUPPORT_SLE_CENTRAL', 'CONFIG_BG_SLE_ONLY']
    },
    'tr5310-s-ble-peripheral': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5310-s-libgen',
        'defines': ['SUPPORT_BLE_PERIPHERAL']
    },
    'tr5310-s-sle-measure-dis': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle', 'cal_dis'],
        'base_target_name': 'tr5310-s-libgen-dis',
        'defines': ['PRODUCT_SLE_MEASURE_DIS',
                    'SLEM_CARKEY', 'MEASURE_DIS', 'EM_32K_SUPPORT']
    },
}