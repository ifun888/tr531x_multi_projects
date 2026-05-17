/**
 * Copyright (c) Triductor 2024-2024. All rights reserved. \n
 *
 * Description: MP Test Header \n
 * Author: Triductor \n
 * History: \n
 * 2024-12-25, Create file. \n
 */
#ifndef MP_TEST_H
#define MP_TEST_H


#include "stdint.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */


#include "sle_common.h"
#include "bts_def.h"


#ifdef CONFIG_SAMPLE_SUPPORT_MOUSE
#define MP_TEST_ADDR_FLASH_START                                0x000FD000
#define MP_TEST_ADDR_FLASH_SIZE                                 0x1000
#else
#define MP_TEST_ADDR_FLASH_START                                0x000FD000
#define MP_TEST_ADDR_FLASH_SIZE                                 0x1000
#endif


#define MP_TEST_ADDR_MAGIC_ID                                   0xAABBCCDD

#define MP_TEST_ADDR_MAGIC_INDEX                                0
#define MP_TEST_ADDR_MAGIC_LEN                                  4
#define MP_TEST_ADDR_SLE_INDEX                                  (MP_TEST_ADDR_MAGIC_INDEX + MP_TEST_ADDR_MAGIC_LEN)
#define MP_TEST_ADDR_SLE_LEN                                    SLE_ADDR_LEN
#define MP_TEST_ADDR_BLE_INDEX                                  (MP_TEST_ADDR_SLE_INDEX + MP_TEST_ADDR_SLE_LEN)
#define MP_TEST_ADDR_BLE_LEN                                    BD_ADDR_LEN


typedef struct
{   
    uint32_t magic_id;
    uint8_t sle_addr[SLE_ADDR_LEN];
    uint8_t ble_addr[BD_ADDR_LEN];    
}mp_test_addr_t;


void mp_test_read_magic_id(uint32_t *magic_id);
void mp_test_read_sle_addr(uint8_t *sle_addr);
void mp_test_read_ble_addr(uint8_t *ble_addr);
void mp_test_set_addr(uint8_t *p_value);


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif