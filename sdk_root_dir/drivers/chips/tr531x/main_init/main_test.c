/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: Application core testsuite && AT init function \n
 *
 * History: \n
 * 2024-03-28, Create file. \n
 */
#ifdef TEST_SUITE
#include "timer.h"
#include "chip_core_irq.h"
#include "los_hwi.h"
#include "test_suite.h"
#include "test_suite_uart.h"
#include "test_auxiliary.h"
#include "chip_io.h"
#include "osal_debug.h"
#endif

#if defined(DEVICE_ONLY) && !defined(TEST_SUITE)
#include "wvt_uart.h"
#ifdef CONFIG_DRIVERS_USB_SERIAL_GADGET
#include "osal_task.h"
#include "gadget/usbd_acm.h"
#include "console.h"
#include "dts_hci_dev_only.h"
#include "implementation/usb_init.h"
#endif
#ifdef CONFIG_DRIVERS_USB_DFU_GADGET
#include "main_dfu.h"
#endif
#endif

#ifdef TEST_SUITE
#include "hal_reboot.h"
#include "key_id.h"
#include "common.h"
#include "nv.h"
#ifdef PLT_TEST_ENABLE
#include "test_keyscan.h"
#include "test_i2s.h"
#include "test_qdec.h"
#include "test_usb.h"
#include "test_pdm.h"
#endif
#ifdef FTRACE
#include "test_ftrace.h"
#endif

#ifdef COREMARK_TEST
#include "core_portme.h"
#endif
#if (ENABLE_LOW_POWER == YES)
#include "pm_veto.h"
#endif
#include "memory_info.h"
#endif

#ifdef TESTSUIT_POS_DIS_ENABLE
#include "test_pos_dis.h"
#endif
#if defined(SUPPORT_CARKEY)
#include "carkey_api.h"
#include "carkey_uapi.h"
#endif

#ifdef AT_COMMAND
#include "at_product.h"
#include "at_porting.h"
#endif

#ifdef NFC_TASK_EXIST
void add_nfc_t2t_test_case(void);
#endif

#if defined(DEVICE_ONLY) && !defined(TEST_SUITE)
#ifdef CONFIG_DRIVERS_USB_SERIAL_GADGET
#define SERIAL_MANUFACTURER  { 'H', 0, 'H', 0, 'H', 0, 'H', 0, 'l', 0, 'i', 0, 'c', 0, 'o', 0, 'n', 0 }
#define SERIAL_MANUFACTURER_LEN   20
#define SERIAL_PRODUCT  { 'H', 0, 'H', 0, '6', 0, '6', 0, '6', 0, '6', 0, ' ', 0, 'U', 0, 'S', 0, 'B', 0 }
#define SERIAL_PRODUCT_LEN        22
#define SERIAL_SERIAL   { '2', 0, '0', 0, '2', 0, '0', 0, '0', 0, '6', 0, '2', 0, '4', 0 }
#define SERIAL_SERIAL_LEN           16
#define SERIAL_DELAY_MS             (1000)
#define SERIAL_RECV_DATA_MAX_LEN    64

static char g_usb_serial_recv_data[SERIAL_RECV_DATA_MAX_LEN];

int usb_serial_receive_data(void *data)
{
    unused(data);
    usb_serial_ioctl(0, CONSOLE_CMD_RD_BLOCK_SERIAL, 1);
    for (;;) {
        ssize_t recv_len = usb_serial_read(0, g_usb_serial_recv_data, SERIAL_RECV_DATA_MAX_LEN);
        if (recv_len <= 0) {
            osal_msleep(SERIAL_DELAY_MS);
            continue;
        }

        for (uint8_t i = 0; i < recv_len; i++) {
            device_only_get_data_from_uart((const void *)(&(g_usb_serial_recv_data[i])), 1, true);
        }
    }
    return -1;
}

int wvt_usb_serial_init(void)
{
    int ret;
    const char manufacturer[SERIAL_MANUFACTURER_LEN] = SERIAL_MANUFACTURER;
    struct device_string str_manufacturer = {
        .str = manufacturer,
        .len = SERIAL_MANUFACTURER_LEN
    };

    const char product[SERIAL_PRODUCT_LEN] = SERIAL_PRODUCT;
    struct device_string str_product = {
        .str = product,
        .len = SERIAL_PRODUCT_LEN
    };

    const char serial[SERIAL_SERIAL_LEN] = SERIAL_SERIAL;
    struct device_string str_serial_number = {
        .str = serial,
        .len = SERIAL_SERIAL_LEN
    };

    struct device_id dev_id = {
        .vendor_id = 0x1111,
        .product_id = 0x0bcd,
        .release_num = 0x0119
    };
#ifdef CONFIG_DRIVERS_USB_DFU_GADGET
    usb_hid_get_index();
    ret = usbd_set_device_info(DEV_SER_HID, &str_manufacturer, &str_product, &str_serial_number, dev_id);
    if (ret != 0) {
        return -1;
    }

    ret = usb_init(DEVICE, DEV_SER_HID);
    if (ret != 0) {
        return -1;
    }
#else
    ret = usbd_set_device_info(DEV_SERIAL, &str_manufacturer, &str_product, &str_serial_number, dev_id);
    if (ret != 0) {
        return -1;
    }

    ret = usb_init(DEVICE, DEV_SERIAL);
    if (ret != 0) {
        return -1;
    }
#endif
    return 0;
}
#endif
#endif

__attribute__((weak)) void main_test_init(void)
{
#if defined(TEST_SUITE)
    uapi_timer_init();
    uapi_timer_adapter(TIMER_INDEX_0, TIMER_0_IRQN, OS_HWI_PRIO_LOWEST - 1);
    test_suite_uart_init();
    uapi_test_suite_init();
#elif defined(AT_COMMAND)
    uapi_at_cmd_init();
#endif

#if defined(DEVICE_ONLY) && !defined(TEST_SUITE)
    wvt_uart_init();
#ifdef CONFIG_DRIVERS_USB_SERIAL_GADGET
    wvt_usb_serial_init();
#endif
#endif
}

#ifdef TEST_SUITE
#if (ENABLE_LOW_POWER == YES)
static int test_mcu_vote_sleep(int argc, char* argv[])
{
    unused(argc);
    uint8_t vote = (uint8_t)strtol(argv[0], NULL, 0);
    if (vote == 0) {
        uapi_pm_add_sleep_veto(PM_VETO_ID_MCU);
        writel(MEMORY_INFO_CRTL_REG, MEMORY_INFO_CLOSE);
    } else {
        uapi_pm_remove_sleep_veto(PM_VETO_ID_MCU);
    }
    return 0;
}
#endif

static int test_reboot_chip(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);
    hal_reboot_chip();
    return AT_RET_OK;
}

static int test_set_own_addr(int argc, char **argv)
{
    if(argc != BD_ADDR_LEN) {
        return AT_RET_OK;
    }
    uint16_t key = SLE_SAMPLE_NV_ID;
    uint16_t key_len = (uint16_t)sizeof(sle_sample_data_config_stru_t);
    uint16_t real_len = 0;
    uint8_t *read_value = osal_vmalloc(key_len);
    if (uapi_nv_read(key, key_len, &real_len, read_value) != ERRCODE_SUCC) {
        /* ERROR PROCESS */
        osal_vfree(read_value);
        read_value = NULL;
        return AT_RET_PROGRESS_BLOCK;
    }
    sle_sample_data_config_stru_t *sle_sample_data_t = (sle_sample_data_config_stru_t *)read_value;
    for(uint8_t i = 0; i< argc; i++){
        uint8_t input_data = (uint8_t)strtol(argv[i], NULL, 0);
        sle_sample_data_t->sle_own_addr[i] = input_data;
    }
   
    errcode_t nv_ret_value = uapi_nv_write(key, read_value, key_len);
    for (uint8_t i = 0; i < key_len; i++) {
        osal_printk("[NV]:write_value:0x%02x \r\n", *(read_value + i));
    }
    if (nv_ret_value != ERRCODE_SUCC) {
        /* ERROR PROCESS */
        osal_printk("[ERROR]write nv fail! ret:%x \r\n", nv_ret_value);
        osal_vfree(read_value);
        read_value = NULL;
        return AT_RET_PROGRESS_BLOCK;
    }
    if (read_value != NULL) {
        osal_vfree(read_value);
        read_value = NULL;
    }
    return AT_RET_OK;
}

static int test_set_remote_addr(int argc, char **argv)
{
    if(argc != BD_ADDR_LEN + 1) {
        return AT_RET_OK;
    }
    uint8_t server_index = (uint8_t)strtol(argv[0], NULL, 0);
    if(server_index > SLE_MULTICON_MAX_NUM - 1){
        return AT_RET_OK;
    }
    uint16_t key = SLE_SAMPLE_NV_ID;
    uint16_t key_len = (uint16_t)sizeof(sle_sample_data_config_stru_t);
    uint16_t real_len = 0;
    uint8_t *read_value = osal_vmalloc(key_len);
    if (uapi_nv_read(key, key_len, &real_len, read_value) != ERRCODE_SUCC) {
        /* ERROR PROCESS */
        osal_vfree(read_value);
        read_value = NULL;
        return AT_RET_PROGRESS_BLOCK;
    }
    sle_sample_data_config_stru_t *sle_sample_data_t = (sle_sample_data_config_stru_t *)read_value;
    for(uint8_t i = 1; i< argc; i++){
        uint8_t input_data = (uint8_t)strtol(argv[i], NULL, 0);
        sle_sample_data_t->sle_remote_addr[server_index][i-1] = input_data;
    }
    sle_sample_data_t->sle_multicon_num = (sle_sample_data_t->sle_multicon_num < (server_index + 1)?(server_index + 1):(sle_sample_data_t->sle_multicon_num));
    errcode_t nv_ret_value = uapi_nv_write(key, read_value, key_len);
    for (uint8_t i = 0; i < key_len; i++) {
        osal_printk("[NV]:write_value:0x%02x \r\n", *(read_value + i));
    }
    if (nv_ret_value != ERRCODE_SUCC) {
        /* ERROR PROCESS */
        osal_printk("[ERROR]write nv fail! ret:%x \r\n", nv_ret_value);
        osal_vfree(read_value);
        read_value = NULL;
        return AT_RET_PROGRESS_BLOCK;
    }
    if (read_value != NULL) {
        osal_vfree(read_value);
        read_value = NULL;
    }
    return AT_RET_OK;
}

static int test_set_adv_name(int argc, char **argv)
{
    if(argc == 0) {
        return AT_RET_OK;
    }
    uint16_t key = SLE_SAMPLE_NV_ID;
    uint16_t key_len = (uint16_t)sizeof(sle_sample_data_config_stru_t);
    uint16_t real_len = 0;
    uint8_t *read_value = osal_vmalloc(key_len);
    if (uapi_nv_read(key, key_len, &real_len, read_value) != ERRCODE_SUCC) {
        /* ERROR PROCESS */
        osal_vfree(read_value);
        read_value = NULL;
        return AT_RET_PROGRESS_BLOCK;
    }
    sle_sample_data_config_stru_t *sle_sample_data_t = (sle_sample_data_config_stru_t *)read_value;
    (void)memcpy_s(sle_sample_data_t->sle_name, strlen(*argv), *argv, strlen(*argv));
    sle_sample_data_t->sle_name[strlen(*argv)] = '\0';
    errcode_t nv_ret_value = uapi_nv_write(key, read_value, key_len);
    for (uint8_t i = 0; i < key_len; i++) {
        osal_printk("[NV]:write_value:0x%02x \r\n", *(read_value + i));
    }
    if (nv_ret_value != ERRCODE_SUCC) {
        /* ERROR PROCESS */
        osal_printk("[ERROR]write nv fail! ret:%x \r\n", nv_ret_value);
        osal_vfree(read_value);
        read_value = NULL;
        return AT_RET_PROGRESS_BLOCK;
    }
    if (read_value != NULL) {
        osal_vfree(read_value);
        read_value = NULL;
    }
    return AT_RET_OK;
}

__attribute__((weak)) void cmd_main_add_functions(void)
{
    add_auxiliary_functions();
	uapi_test_suite_add_function("reboot", "<reboot>", test_reboot_chip);
	uapi_test_suite_add_function("set_sle_own_addr", "Six Params: <Addr>, ...,<Addr> (hex or decimal)", test_set_own_addr);
    uapi_test_suite_add_function("set_sle_remote_addr", "Seven Params:index,<Addr>, ...,<Addr> (hex or decimal)", test_set_remote_addr);
    uapi_test_suite_add_function("set_sle_adv_name", "Params: nameString(no less than 32)", test_set_adv_name);
#ifdef AT_COMMAND
    uapi_test_suite_add_function("testsuite_sw_at", "<at>", uapi_testsuite_sw_at);
#endif
#ifdef COREMARK_TEST
    uapi_test_suite_add_function("coremark", "Coremark test Function", coremark_test);
#endif
#if (ENABLE_LOW_POWER == YES)
    uapi_test_suite_add_function("mcu_vote_slp", "MCU vote to sleep or not Function", test_mcu_vote_sleep);
#endif
#ifdef PLT_TEST_ENABLE
#ifndef CONFIG_BT_UPG_ENABLE
    add_usb_test_case();
#endif
#endif
#ifdef NFC_TASK_EXIST
    add_nfc_t2t_test_case();
#endif
#ifdef FTRACE
    add_ftrace_test_case();
#endif
#ifdef TESTSUIT_POS_DIS_ENABLE
    add_pos_dis_test_case();
#endif
#ifdef SUPPORT_CARKEY
    add_sle_slem_nv_base_test_case();
    add_sle_slem_nv_alg_test_case();
#endif
}
#endif