#!/usr/bin/env python3
# encoding=utf-8
# ============================================================================
# @brief    Target Definitions File
# Copyright Triductor 2022-2022. All rights reserved.
# ============================================================================

tr5316_s_libs = {
    'tr5316-s-sle-peripheral': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5316-s-libgen',
        'defines': ['SUPPORT_SLE_PERIPHERAL', 'CONFIG_BG_SLE_ONLY']
    },
    'tr5316-s-sle-ble-peripheral': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5316-s-libgen',
        'defines': ['SUPPORT_SLE_BLE_PERIPHERAL']
    },
    'tr5316-s-sle-central': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5316-s-libgen',
        'defines': ['SUPPORT_SLE_CENTRAL', 'CONFIG_BG_SLE_ONLY']
    },
    'tr5316-s-ble-peripheral': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5316-s-libgen',
        'defines': ['SUPPORT_BLE_PERIPHERAL']
    },
    'tr5316-s-sle-peripheral-release': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5316-s-libgen',
        'defines': ['SUPPORT_SLE_PERIPHERAL', 'CONFIG_BG_SLE_ONLY',  'BT_USER_RELEASE']
    },
    'tr5316-s-sle-ble-peripheral-release': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5316-s-libgen',
        'defines': ['SUPPORT_SLE_BLE_PERIPHERAL', 'BT_USER_RELEASE']
    },
    'tr5316-s-sle-central-release': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5316-s-libgen',
        'defines': ['SUPPORT_SLE_CENTRAL', 'CONFIG_BG_SLE_ONLY', 'BT_USER_RELEASE']
    },
    'tr5316-s-ble-peripheral-release': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5316-s-libgen',
        'defines': ['SUPPORT_BLE_PERIPHERAL', 'BT_USER_RELEASE']
    }
}
