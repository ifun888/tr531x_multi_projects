/**
 * Copyright (c) Triductor 2024-2024. All rights reserved. \n
 *
 * Description: MP Test Source. \n
 * Author: Triductor \n
 * History: \n
 * 2024-12-25, Create file. \n
 */
#include "stdbool.h"
#include "string.h"
#include "sfc.h"
#include "osal_debug.h"
#include "mp_test.h"


static void mp_test_addr_erase(void)
{
    osal_printk("Erasing for MP Test ADDR...\r\n");
    errcode_t ret = uapi_sfc_reg_erase(MP_TEST_ADDR_FLASH_START, MP_TEST_ADDR_FLASH_SIZE);
    if (ret != ERRCODE_SUCC) 
    {
        osal_printk("mp_test_addr_erase failed! ret = %x\r\n", ret);
        return;
    }
}

static void mp_test_addr_write(uint8_t index, void *p_value, uint16_t len)
{
    errcode_t ret = uapi_sfc_reg_write(MP_TEST_ADDR_FLASH_START + index, p_value, len);
    if (ret != ERRCODE_SUCC) 
    {
        osal_printk("mp_test_addr_write failed! ret = %x\r\n", ret);
        return;
    }
}

static void mp_test_addr_read(uint8_t index, void *p_value, uint16_t len)
{
    errcode_t ret = uapi_sfc_reg_read(MP_TEST_ADDR_FLASH_START + index, p_value, len);
    if (ret != ERRCODE_SUCC) 
    {
        osal_printk("mp_test_addr_read failed! ret = %x\r\n", ret);
        return;
    }
}

void mp_test_write_magic_id(uint32_t *magic_id)
{
    mp_test_addr_write(MP_TEST_ADDR_MAGIC_INDEX, magic_id, MP_TEST_ADDR_MAGIC_LEN);
}

void mp_test_write_sle_addr(uint8_t *sle_addr)
{
    mp_test_addr_write(MP_TEST_ADDR_SLE_INDEX, sle_addr, MP_TEST_ADDR_SLE_LEN);
}

void mp_test_write_ble_addr(uint8_t *ble_addr)
{
    mp_test_addr_write(MP_TEST_ADDR_BLE_INDEX, ble_addr, MP_TEST_ADDR_BLE_LEN);
}

void mp_test_read_magic_id(uint32_t *magic_id)
{
    mp_test_addr_read(MP_TEST_ADDR_MAGIC_INDEX, magic_id, MP_TEST_ADDR_MAGIC_LEN);
}

void mp_test_read_sle_addr(uint8_t *sle_addr)
{
    mp_test_addr_read(MP_TEST_ADDR_SLE_INDEX, sle_addr, MP_TEST_ADDR_SLE_LEN);
}

void mp_test_read_ble_addr(uint8_t *ble_addr)
{
    mp_test_addr_read(MP_TEST_ADDR_BLE_INDEX, ble_addr, MP_TEST_ADDR_BLE_LEN);
}

void mp_test_set_addr(uint8_t *p_value)
{
    mp_test_addr_t mp_test_addr = {0};
    uint32_t magic_id = MP_TEST_ADDR_MAGIC_ID;

    mp_test_addr_erase();

    mp_test_write_sle_addr(&p_value[0]);
    mp_test_write_ble_addr(&p_value[SLE_ADDR_LEN]);
    mp_test_write_magic_id(&magic_id);

    mp_test_read_magic_id(&mp_test_addr.magic_id);
    mp_test_read_sle_addr(mp_test_addr.sle_addr);
    mp_test_read_ble_addr(mp_test_addr.ble_addr);

    osal_printk("sle addr: ");
    for(uint8_t i = 0; i < SLE_ADDR_LEN; i++)
    {
        osal_printk("0x%02x ", mp_test_addr.sle_addr[i]);
    }
    osal_printk("\r\n");

    osal_printk("ble addr: ");
    for(uint8_t i = 0; i < BD_ADDR_LEN; i++)
    {
        osal_printk("0x%02x ", mp_test_addr.ble_addr[i]);
    }
    osal_printk("\r\n");
}