#!/usr/bin/env python3
# encoding=utf-8
# ============================================================================
# @brief    Target Definitions File
# Copyright Triductor 2022-2022. All rights reserved.
# ============================================================================
# sdk onetrack depend libs.
from ..tr5310.sdk_libs import tr5310_s_libs
from ..tr5310pe.sdk_libs import tr5310pe_1100e_libs
from ..tr5312l.sdk_libs import tr5312l_s_libs
from ..tr5316.sdk_libs import tr5316_s_libs

tr5310p_n1100_libs = {
    'tr5310p-n1100-sle-peripheral': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5310p-n1100-libgen',
        'defines': ['SUPPORT_SLE_PERIPHERAL', 'CONFIG_BG_SLE_ONLY']
    },
    'tr5310p-n1100-sle-ble-peripheral': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5310p-n1100-libgen',
        'defines': ['SUPPORT_SLE_BLE_PERIPHERAL']
    },
    'tr5310p-n1100-sle-central': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5310p-n1100-libgen',
        'defines': ['SUPPORT_SLE_CENTRAL', 'CONFIG_BG_SLE_ONLY']
    },
    'tr5310p-n1100-ble-peripheral': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5310p-n1100-libgen',
        'defines': ['SUPPORT_BLE_PERIPHERAL']
    },
    'tr5310p-n1100-sle-peripheral-release': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5310p-n1100-libgen',
        'defines': ['SUPPORT_SLE_PERIPHERAL', 'CONFIG_BG_SLE_ONLY',  'BT_USER_RELEASE']
    },
    'tr5310p-n1100-sle-ble-peripheral-release': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5310p-n1100-libgen',
        'defines': ['SUPPORT_SLE_BLE_PERIPHERAL', 'BT_USER_RELEASE']
    },
    'tr5310p-n1100-sle-central-release': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bt_app'],
        'base_target_name': 'tr5310p-n1100-libgen',
        'defines': ['SUPPORT_SLE_CENTRAL', 'CONFIG_BG_SLE_ONLY', 'BT_USER_RELEASE']
    },
    'tr5310p-n1100-ble-peripheral-release': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle'],
        'base_target_name': 'tr5310p-n1100-libgen',
        'defines': ['SUPPORT_BLE_PERIPHERAL', 'BT_USER_RELEASE']
    }
}

tr5310p_s_libs = {
    'tr5310p-s-sle-peripheral': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bth_sdk', 'bt_app'],
        'base_target_name': 'tr5310p-s-libgen',
        'defines': ['SUPPORT_SLE_PERIPHERAL', 'CONFIG_BG_SLE_ONLY']
    },
    'tr5310p-s-sle-ble-peripheral': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle', 'bth_sdk'],
        'base_target_name': 'tr5310p-s-libgen',
        'defines': ['SUPPORT_SLE_BLE_PERIPHERAL']
    },
    'tr5310p-s-sle-central': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bth_sdk', 'bt_app'],
        'base_target_name': 'tr5310p-s-libgen',
        'defines': ['SUPPORT_SLE_CENTRAL', 'CONFIG_BG_SLE_ONLY']
    },
    'tr5310p-s-ble-peripheral': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle', 'bth_sdk'],
        'base_target_name': 'tr5310p-s-libgen',
        'defines': ['SUPPORT_BLE_PERIPHERAL']
    },
    'tr5310p-s-sle-peripheral-release': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bth_sdk', 'bt_app'],
        'base_target_name': 'tr5310p-s-libgen',
        'defines': ['SUPPORT_SLE_PERIPHERAL', 'CONFIG_BG_SLE_ONLY',  'BT_USER_RELEASE']
    },
    'tr5310p-s-sle-ble-peripheral-release': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle', 'bth_sdk'],
        'base_target_name': 'tr5310p-s-libgen',
        'defines': ['SUPPORT_SLE_BLE_PERIPHERAL', 'BT_USER_RELEASE']
    },
    'tr5310p-s-sle-central-release': {
        'components': ['bg_common', 'bth_gle', 'bgtp', 'bt_host', 'bth_sdk', 'bt_app'],
        'base_target_name': 'tr5310p-s-libgen',
        'defines': ['SUPPORT_SLE_CENTRAL', 'CONFIG_BG_SLE_ONLY', 'BT_USER_RELEASE']
    },
    'tr5310p-s-ble-peripheral-release': {
        'components': ['bgtp', 'bg_common', 'bt_host', 'bt_app', 'bth_gle', 'bth_sdk'],
        'base_target_name': 'tr5310p-s-libgen',
        'defines': ['SUPPORT_BLE_PERIPHERAL', 'BT_USER_RELEASE']
    }
}

tr531x_ci_sdk_libs = {
    **tr5310p_n1100_libs, **tr5310pe_1100e_libs
}

tr531x_sdk_libs = {
    **tr5310_s_libs,
    **tr5310p_n1100_libs,
    **tr5310pe_1100e_libs,
    **tr5312l_s_libs,
    **tr5316_s_libs,
}

tr531x_cy_sdk_libs = {
    **tr5310_s_libs,
    **tr5310p_s_libs,
    **tr5312l_s_libs,
    **tr5316_s_libs,
}
