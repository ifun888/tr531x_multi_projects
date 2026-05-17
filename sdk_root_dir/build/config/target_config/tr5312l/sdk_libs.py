#!/usr/bin/env python3
# encoding=utf-8
# ============================================================================
# @brief    Target Definitions File
# Copyright Triductor 2022-2022. All rights reserved.
# ============================================================================

tr5312l_s_libs = {
    'tr5312l-s-sle-peripheral': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5312l-s-libgen',
        'defines': ['SUPPORT_SLE_PERIPHERAL', 'CONFIG_BG_SLE_ONLY']
    },
    'tr5312l-s-sle-ble-peripheral': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5312l-s-libgen',
        'defines': ['SUPPORT_SLE_BLE_PERIPHERAL']
    },
    'tr5312l-s-sle-central': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5312l-s-libgen',
        'defines': ['SUPPORT_SLE_CENTRAL', 'CONFIG_BG_SLE_ONLY']
    },
    'tr5312l-s-ble-peripheral': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5312l-s-libgen',
        'defines': ['SUPPORT_BLE_PERIPHERAL']
    }
}