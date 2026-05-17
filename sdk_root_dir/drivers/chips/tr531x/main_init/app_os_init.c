/**
 * Copyright (c) Triductor. 2022-2023. All rights reserved.
 *
 * Description: Application core os initialize interface for standard \n
 *
 * History: \n
 * 2022-07-27, Create file. \n
 */

#include "soc_osal.h"
#include "log_common.h"
#include "log_def.h"
#include "log_uart.h"
#include "pmu_interrupt.h"
#include "log_reg_dump.h"
#include "watchdog.h"
#include "preserve.h"
#include "hal_reboot.h"
#ifdef OS_DFX_SUPPORT
#include "os_dfx.h"
#endif
#include "los_task_pri.h"
#include "debug_print.h"
#include "test_usb.h"
#if (PM_MCPU_MIPS_STATISTICS_ENABLE == YES)
#include "pm_porting.h"
#endif

#include "chip_io.h"
#if defined(CONFIG_SAMPLE_ENABLE)
#include "app_init.h"
#endif
#if defined(SUPPORT_CARKEY)
#include "carkey_api.h"
#include "carkey_uapi.h"
#endif
#ifdef AT_COMMAND
#include "at_product.h"
#include "at_porting.h"
#endif
#if defined (TEST_SUITE) || defined (CONFIG_DRIVERS_USB_SERIAL_GADGET)
#include "main_test.h"
#endif
#if defined (DEVICE_ONLY) && defined (CONFIG_DRIVERS_USB_DFU_GADGET)
#include "main_dfu.h"
#endif
#include "memory_info.h"
#if defined(CONFIG_PM_SYS_SUPPORT)
#include "pm_sys.h"
#endif
#include "log_buffer_reader.h"
#include "diag_adapt_sdt.h"
#include "app_os_init.h"

#define KICK_DOG_INTERVAL_MS        9000
#define HSO_LOG_MSG_LEN             1
#define HSO_LOG_MSG                 0xff
#define MSG_MAX_SIZE                8
#define MSG_MAX_LEN                 10

/*
 *  优先级范围 0-31
 *  0：     SWT 线程优先级  （最高 不可更改）。
 *  1-10    协议栈线程优先级 （不可更改）。
 *  11-29   用户可使用。
 *  30      平台低功耗 （不可更改）。
 *  31：    IDLE 线程优先级（最低 不可更改）。
 */
/* 平台线程优先级 */
#define TASK_PRIORITY_CMD                   20
#define TASK_PRIORITY_APP                   21
#define TASK_PRIORITY_LOG                   29
/* 业务线程优先级 */
/* controller task */
#define TASK_PRIORITY_BT                    3
/* host task. */
#define TASK_PRIORITY_SCH                   4
#define TASK_PRIORITY_SRV                   5
#define TASK_PRIORITY_HADM                  27

#ifdef SLEM_CARKEY
#define APP_STACK_SIZE                      0x800
#define BT_STACK_SIZE                       0xA00
/* host schedule task stack. */
#define BTH_SCHEDULE_STACK_SIZE             0xC00
/* host service task stack. */
#define BTH_SERVICE_STACK_SIZE              0x900
/* host measure task stack. */
#ifdef CONFIG_SINGLE_CARKEY
#define BTH_HADM_SERVICE_STACK_SIZE         0xA00
#else
#define BTH_HADM_SERVICE_STACK_SIZE         0x1100
#endif
#else
/* 线程栈分配，推荐比水线高30%  */
#define APP_STACK_SIZE                      0xA00
#define BT_STACK_SIZE                       0xA00
/* host schedule task stack. */
#define BTH_SCHEDULE_STACK_SIZE             0xC00
/* host service task stack. */
#define BTH_SERVICE_STACK_SIZE              0xC00
#ifdef CONFIG_DRIVERS_USB_SERIAL_GADGET
#define SERIAL_STACK_SIZE                   0x400
#endif
#endif

#define TASK_COMMON_APP_DELAY_MS            20000

typedef struct {
    char *task_name;
    void *task_arg;
    uint32_t task_stack;
    uint32_t task_pri;
    osal_kthread_handler task_func;
} app_task_attr_t;

typedef struct {
    int task_prio;
    int stack_size;
} task_attr_t;

/* plt init thread. */
__attribute__((weak)) void app_main(void *unused);
/* rf init thread. */
void bt_thread_handle(void *para);

#ifndef DEVICE_ONLY
void bt_acore_task_main(const void *pvParams);
void bt_tran_task_queue_init(void);
void recv_data_task(void);
void btsrv_task_body(const void *data);
void sdk_msg_thread(void);
#endif
#if defined(CONFIG_SLE_AMIC_TRANS_PATH_CHECK)
void sle_vdt_get_trans_path_value(void);
#endif
void at_channel_check_and_enable(void);
void at_msg_process(void *msg);
static unsigned long g_uart_msg_queue;
/* bth schedule task attr. */
const task_attr_t g_bth_schedule_attr = {
    .task_prio = TASK_PRIORITY_SCH,
    .stack_size = BTH_SCHEDULE_STACK_SIZE,
};

/*
 *  开启内存维测：
 *  将 kernel\liteos\liteos_v208.6.0_b017\Huawei_LiteOS\tools\build\config\tr531x_xxx.config 文件内下列配置设置为 y。
 *      LOSCFG_MEM_TASK_STAT=y
 *      LOSCFG_MEM_DFX_SHOW_CALLER_RA=y
 *  查看内存维测信息：
 *      AT+TASKSTACK：查看所有线程栈使用信息，若线程栈空闲较多，建议减小栈大小，节省内存空间。
 *      AT+TASKMALLOC：查看所有线程内存堆申请信息，若线程堆申请较大，建议排查线程调用内存申请释放接口是否合理。
 *      AT+HEAPSTAT：查看堆整体使用信息。
 *  线程栈溢出常用分析方法：
 *      1）使用AT+TASKSTACK观察栈使用峰值是否接近栈顶。
 *      2）异常死机时查看串口打印的栈信息log，如果PeakUsage大于或者接近栈大小，则可能存在栈溢出。
 */

/*
 *  线程创建数组，数量上限通过：
 *  kernel\liteos\liteos_v208.6.0_b017\Huawei_LiteOS\tools\build\config\tr531x_xxx.config
 *  文件内的 LOSCFG_BASE_CORE_TSK_LIMIT=* 配置项进行配置。
 *  注意：线程越多，消耗内存越多，请合理配置使用。
 *  当前数组定义了6个线程:
 *      app: 平台主线程，用于创建Sample任务、LOG打印、执行AT命令，低功耗sample状态机pm_sys的状态转移也在此任务中执行。
 *      bt: 协议栈controller层主线程（不可修改）。
 *      bt_service: 协议栈host层主线程（不可修改），线程内部创建bth_schedule线程（不可修改）。
 *      key: 星闪测距线程（可根据实际需求裁剪）。
 *      WVTSerialTask：Device Only版本测试用线程，支持用虚拟串口收发wvt命令，默认不开启（用户可修改）。
 *      WVTDFUTask：Device Only版本测试用线程，支持DFU升级，默认不开启（用户可修改）。
 *  os隐式创建2个线程：SWT、IDLE（仅支持修改线程栈）。
 *      线程栈修改方式：修改kernel\liteos\liteos_v208.6.0_b017\Huawei_LiteOS\tools\build\config\tr531x_xxx.config文件内
 *                      LOSCFG_BASE_CORE_TSK_SWTMR_STACK_SIZE、LOSCFG_BASE_CORE_TSK_IDLE_STACK_SIZE配置项大小。
 *  平台隐式创建1个线程：PM_ENTRY（仅支持修改线程栈）。
 *  包含测距业务总共创建10个线程，不包含总共创建9个线程。
 */
static const app_task_attr_t g_app_tasks[] = {
#ifdef BGLE_TASK_EXIST
    {"bt", NULL, BT_STACK_SIZE, TASK_PRIORITY_BT, (osal_kthread_handler)bt_thread_handle},
#ifndef DEVICE_ONLY
    {"bt_service", (void *)&g_bth_schedule_attr, BTH_SERVICE_STACK_SIZE, TASK_PRIORITY_SRV,
     (osal_kthread_handler)btsrv_task_body},
#else
#ifdef CONFIG_DRIVERS_USB_SERIAL_GADGET
    {"WVTSerialTask", NULL, SERIAL_STACK_SIZE, TASK_PRIORITY_LOG, (osal_kthread_handler)usb_serial_receive_data},
#endif
#ifdef CONFIG_DRIVERS_USB_DFU_GADGET
    {"WVTDFUTask", NULL, SERIAL_STACK_SIZE, TASK_PRIORITY_LOG - 1, (osal_kthread_handler)usb_hid_recv_data},
#endif
#endif
#ifdef SUPPORT_CARKEY
    {"key", NULL, BTH_HADM_SERVICE_STACK_SIZE, TASK_PRIORITY_HADM, (osal_kthread_handler)key_main},
#endif /* SUPPROT_CARKEY */
#endif
    {"app", NULL, APP_STACK_SIZE, TASK_PRIORITY_APP, (osal_kthread_handler)app_main},
};

#define M_NUM_TASKS (sizeof(g_app_tasks) / sizeof(app_task_attr_t))

void app_os_init(void)
{
    osal_task *cur_handle = NULL;
    /* 线程创建接口 */
    osal_kthread_lock();
    for (uint8_t i = 0; i < M_NUM_TASKS; i++) {
        cur_handle = osal_kthread_create(g_app_tasks[i].task_func, g_app_tasks[i].task_arg,
                                         g_app_tasks[i].task_name, g_app_tasks[i].task_stack);
        if (cur_handle == NULL) {
            panic(PANIC_TASK_CREATE_FAILED, i);
        }
        osal_kthread_set_priority(cur_handle, g_app_tasks[i].task_pri);
        /* AT命令线程 */
#ifdef TEST_SUITE
#ifdef AT_COMMAND
        if (strcmp(g_app_tasks[i].task_name, "at") == 0) {
            uapi_set_at_task((uint32_t *)cur_handle->task);
            osal_kthread_suspend(cur_handle);
        }
#endif
#endif
    }
    osal_kthread_unlock();
#ifdef TEST_SUITE
    cmd_main_add_functions();
#endif
}

#if defined (CONFIG_SUPPORT_LOG_THREAD)
void write_hso_log_msg(void)
{
    uint8_t msg = HSO_LOG_MSG;
    osal_msg_queue_write_copy(g_uart_msg_queue, &msg, HSO_LOG_MSG_LEN, LOS_NO_WAIT);
}
#endif

static void app_main_task_init(void)
{
    hal_reboot_clear_history();
    system_boot_reason_print();
    system_boot_reason_process();
#if (USE_COMPRESS_LOG_INSTEAD_OF_SDT_LOG == NO)
    log_exception_dump_reg_check();
#endif
#if defined(CONFIG_SAMPLE_ENABLE)
    app_tasks_init();
#endif
#ifdef OS_DFX_SUPPORT
    print_os_task_id_and_name();
#endif
#ifdef AT_COMMAND
    at_channel_check_and_enable();
    uapi_at_register_custom_msg_queue(g_uart_msg_queue);
#endif
#if defined (CONFIG_SUPPORT_LOG_THREAD)
    register_log_trigger(write_hso_log_msg);
#endif
#if defined(CONFIG_PM_SYS_SUPPORT)
    pm_sys_entry();
#endif
    add_usb_test_case();
}

__attribute__((weak)) void app_main(void *unused)
{
    unused(unused);
    uint8_t msg[MSG_MAX_LEN];
    uint32_t msg_size = sizeof(msg);
    log_reader_ret_t lr_ret;
    log_memory_region_section_t lregion;
    log_buffer_header_t lb_header = { 0 };
    uint8_t *b1 = NULL;
    uint32_t l1 = 0;
    uint8_t *b2 = NULL;
    uint32_t l2 = 0;
    osal_msg_queue_create(NULL, (unsigned short)MSG_MAX_LEN, &g_uart_msg_queue, 0, sizeof(msg));
    app_main_task_init();
    while (1) {
#if defined (CONFIG_SUPPORT_LOG_THREAD)
        // Check if there are messages
        while (log_buffer_reader_lock_next(&lregion, &lb_header) == LOG_READER_RET_OK) {
            // Claim the message available
            lr_ret = log_buffer_reader_claim_next(lregion, &b1, &l1, &b2, &l2);
            // we are sure there is a new message
            if ((lr_ret != LOG_READER_RET_OK) || ((lb_header.length - sizeof(lb_header)) != (l1 + l2))) {
                log_buffer_reader_error_recovery(lregion);
                break;
            }
            zdiag_adapt_sdt_msg_proc(b1, l1, b2, l2);
            log_buffer_reader_discard(lregion);
            uapi_watchdog_kick();
        }
#endif
        msg_size = sizeof(msg);
        if (osal_msg_queue_read_copy(g_uart_msg_queue, msg, &msg_size, KICK_DOG_INTERVAL_MS) != LOS_OK) {
            uapi_watchdog_kick();
#ifdef CONFIG_SAMPLE_SUPPORT_MOUSE_BODY
            extern void mouse_body_func(void);
            mouse_body_func();
#endif
            continue;
        }
        if (msg_size != HSO_LOG_MSG_LEN) {
#ifdef AT_COMMAND
            at_msg_process(&msg);
#endif
            continue;
        }
//         if (msg[0] != HSO_LOG_MSG) {
// #if defined(CONFIG_PM_SYS_SUPPORT)
//             pm_sys_task(&msg[0]);
// #endif
        // }
    }
}