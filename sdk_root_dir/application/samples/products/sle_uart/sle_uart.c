/**
 * Copyright (c) Triductor. 2023-2023. All rights reserved.
 *
 * Description: SLE UART Sample Source. \n
 *
 * History: \n
 * 2023-07-17, Create file. \n
 */
#include "common_def.h"
#include "soc_osal.h"
#include "app_init.h"
#include "uart.h"
#include "pm_clock.h"
#include "sle_low_latency.h"
#include "test_suite.h"
#include "gpio.h"
#include "string.h"
#include "tcxo.h"
#include "securec.h"
#if defined(CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER)
#include "securec.h"
#include "sle_uart_server.h"
#include "sle_uart_server_adv.h"
#include "sle_device_discovery.h"
#include "sle_errcode.h"
#include "timer.h"
#include "chip_core_irq.h"
#include "arch_port.h"
#elif defined(CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT)
#define SLE_UART_TASK_STACK_SIZE            0x600
#include "sle_connection_manager.h"
#include "sle_ssap_client.h"
#include "sle_uart_client.h"
#endif  /* CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT */

#define SLE_UART_TASK_PRIO                  28
#define SLE_UART_TASK_DURATION_MS           50
#define SLE_UART_AGGREGATE_MAX_LEN          50
#ifdef CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE
#define SLE_UART_RX_TRIGGER_LEN             50
#else
#define SLE_UART_RX_TRIGGER_LEN             1
#endif
//#define SLE_UART_PACK_TEST_MODE   //sle_uart发包测试模式，默认不开启，开启后uart不能发送接收消息
#ifdef SLE_UART_PACK_TEST_MODE
static uint8_t sle_uart_send_buff_test[21] = {0,1,2,3,4,5,6,7,8,9,0,1,2,
        3,4,5,6,7,8,9,0}; //如果需要进行该数组修改，sle_uart_notification_cb中的判断模式条件也得修改
#define SLE_UART_S_MGPIO S_MGPIO10
#endif
#if defined(CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER)
#define SLE_UART_SERVER_DELAY_COUNT         5

#define SLE_UART_TASK_STACK_SIZE            0x1200
#define SLE_ADV_HANDLE_DEFAULT              1
#define SLE_UART_SERVER_MSG_QUEUE_LEN       5
#define SLE_UART_SERVER_MSG_QUEUE_MAX_SIZE  32
#define SLE_UART_SERVER_QUEUE_DELAY         0xFFFFFFFF
#define SLE_UART_SERVER_BUFF_MAX_SIZE       800
#define SLE_UART_SERVER_SEND_BUFF_MAX_LEN   64
unsigned long g_sle_uart_server_msgqueue_id;
#define SLE_UART_SERVER_LOG                 "[sle uart server]"
#ifdef  SLE_UART_PACK_TEST_MODE
#define SLE_UART_PERF_TX_TIMER_1S 1000000
#define SLE_UART_PERF_TX_TIMER_US 20000
#define SLE_UART_SEND_DATA_LEN 300
static uint64_t tx_trace_data = 0;
static timer_handle_t g_uart_perf_timer = NULL;
static uint16_t pkt_len = 21;
static uint8_t pkt_send_mode = 0;	//0:校验接收数据模式；1：测试掉包率模式.
#endif
static void ssaps_server_read_request_cbk(uint8_t server_id, uint16_t conn_id, ssaps_req_read_cb_t *read_cb_para,
    errcode_t status)
{
    osal_printk("%s ssaps read request cbk callback server_id:%x, conn_id:%x, handle:%x, status:%x\r\n",
        SLE_UART_SERVER_LOG, server_id, conn_id, read_cb_para->handle, status);
}
static void ssaps_server_write_request_cbk(uint8_t server_id, uint16_t conn_id, ssaps_req_write_cb_t *write_cb_para,
    errcode_t status)
{
    // osal_printk("%s ssaps write request callback cbk server_id:%x, conn_id:%x, handle:%x, status:%x\r\n",
        // SLE_UART_SERVER_LOG, server_id, conn_id, write_cb_para->handle, status);
    unused(server_id);
    unused(conn_id);
    unused(status);
    if ((write_cb_para->length > 0) && write_cb_para->value) {
        uapi_uart_write(CONFIG_SLE_UART_BUS, (uint8_t *)write_cb_para->value, write_cb_para->length, 0);
    }
}

#ifdef CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE
uint8_t g_buff[SLE_UART_SERVER_BUFF_MAX_SIZE] = {0};
uint16_t g_uart_buff_len = 0;
uint8_t g_buff_data_valid = 0;
uint8_t g_trigger = 0;
static void sle_uart_server_low_latency_recv_data_cbk(uint16_t len, uint8_t *value)
{
    if ((value == NULL) || (len == 0)) {
        return;
    }
    uapi_uart_write(CONFIG_SLE_UART_BUS, value, len, 0);
}

static void sle_uart_server_low_latency_recv_cbk_register(void)
{
    sle_low_latency_rx_callbacks_t cbk_func = {0};
    cbk_func.low_latency_rx_cb = (low_latency_general_rx_callback)sle_uart_server_low_latency_recv_data_cbk;
    sle_low_latency_rx_register_callbacks(&cbk_func);
}
#endif
static void sle_uart_server_read_int_handler(const void *buffer, uint16_t length, bool error)
{
    unused(error);
    if (sle_uart_client_is_connected()) {
#ifdef CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE
    uint16_t tx_len = length;
    if (tx_len > SLE_UART_AGGREGATE_MAX_LEN) {
        tx_len = SLE_UART_AGGREGATE_MAX_LEN;
    }
    g_buff_data_valid = 1;
    g_uart_buff_len = 0;
    if (memcpy_s(g_buff, SLE_UART_SERVER_SEND_BUFF_MAX_LEN, buffer, tx_len) == EOK) {
        g_uart_buff_len = tx_len;
    } else {
        g_buff_data_valid = 0;
    }
#else
    sle_uart_server_send_report_by_handle(buffer, length);
#endif
    } else {
        osal_printk("%s sle client is not connected! \r\n", SLE_UART_SERVER_LOG);
    }
}

#ifdef CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE
uint8_t *sle_uart_low_latency_tx_cbk(uint16_t *len)
{
#ifdef CONFIG_SAMPLE_SUPPORT_PERFORMANCE_TYPE
if (g_trigger) {
    g_uart_buff_len = SLE_UART_SERVER_SEND_BUFF_MAX_LEN;
    g_buff_data_valid = 1;
}
#endif
    if (g_buff_data_valid == 0) {
        return NULL;
    }
    if (g_uart_buff_len == 0) {
        return NULL;
    }
    *len = g_uart_buff_len;
    g_buff_data_valid = 0;
    return g_buff;
}

void sle_uart_low_latency_tx_cbk_register(void)
{
    sle_low_latency_tx_callbacks_t cbk_func = {0};
    cbk_func.low_latency_tx_cb = sle_uart_low_latency_tx_cbk;
    sle_low_latency_tx_register_callbacks(&cbk_func);
}
#endif

static void sle_uart_server_create_msgqueue(void)
{
    if (osal_msg_queue_create("sle_uart_server_msgqueue", SLE_UART_SERVER_MSG_QUEUE_LEN, \
        (unsigned long *)&g_sle_uart_server_msgqueue_id, 0, SLE_UART_SERVER_MSG_QUEUE_MAX_SIZE) != OSAL_SUCCESS) {
        osal_printk("^%s sle_uart_server_create_msgqueue message queue create failed!\n", SLE_UART_SERVER_LOG);
    }
}

static void sle_uart_server_delete_msgqueue(void)
{
    osal_msg_queue_delete(g_sle_uart_server_msgqueue_id);
}

static void sle_uart_server_write_msgqueue(uint8_t *buffer_addr, uint16_t buffer_size)
{
    osal_msg_queue_write_copy(g_sle_uart_server_msgqueue_id, (void *)buffer_addr, \
                              (uint32_t)buffer_size, 0);
}

static int32_t sle_uart_server_receive_msgqueue(uint8_t *buffer_addr, uint32_t *buffer_size)
{
    return osal_msg_queue_read_copy(g_sle_uart_server_msgqueue_id, (void *)buffer_addr, \
                                    buffer_size, SLE_UART_SERVER_QUEUE_DELAY);
}
static void sle_uart_server_rx_buf_init(uint8_t *buffer_addr, uint32_t *buffer_size)
{
    *buffer_size = SLE_UART_SERVER_MSG_QUEUE_MAX_SIZE;
    (void)memset_s(buffer_addr, *buffer_size, 0, *buffer_size);
}

#ifdef SLE_UART_PACK_TEST_MODE
static void uart_perf_timer_init(void)
{
    uapi_timer_adapter(TIMER_INDEX_1, TIMER_1_IRQN, irq_prio(TIMER_1_IRQN));
    uapi_timer_create(TIMER_INDEX_1, &g_uart_perf_timer);
}

static int pkt_len_change(int argc, char *argv[])
{
    if (1 != argc) {
        osal_printk("Invalid param input!\r\n");
        return 1;
    }
    pkt_len = (uint32_t)strtol(argv[0], NULL, 0);
    osal_printk("the input argc :%d argv:%s pkt_len:%d\r\n", argc, argv[0], pkt_len);
    return 0;
}

static int pkt_send_mode_change(int argc, char *argv[])
{
    if (1 != argc) {
        osal_printk("Invalid param input!\r\n");
        return 1;
    }
    pkt_send_mode = (uint32_t)strtol(argv[0], NULL, 0);
    osal_printk("the input argc :%d argv:%s pkt_send_mode:%d\r\n", argc, argv[0], pkt_send_mode);
    return 0;
}

static void at_send_pkt(uintptr_t data)
{
    uint32_t cycle_num = (uint32_t)data;
    uint8_t sle_send_buff[SLE_UART_SEND_DATA_LEN] = { 0 };
    if (sle_uart_client_is_connected()) {
        if (pkt_send_mode == 0) {
            memcpy_s((uint8_t *)sle_send_buff, sizeof(sle_uart_send_buff_test), (uint8_t *)&sle_uart_send_buff_test, sizeof(sle_uart_send_buff_test));
        } else {
            memcpy_s((uint8_t *)sle_send_buff, sizeof(uint64_t), (uint8_t *)&tx_trace_data, sizeof(uint64_t));
        }
        sle_uart_server_read_int_handler(sle_send_buff, pkt_len, 0);
        tx_trace_data++;
    }

    if ((0 == tx_trace_data) || (0 == cycle_num)) {
        uapi_gpio_toggle(SLE_UART_S_MGPIO);
    }

    cycle_num--;
    if (0 == cycle_num) {
        tx_trace_data = 0;
        uapi_timer_stop(g_uart_perf_timer);
    } else {
        if (pkt_send_mode == 0) {
            uapi_timer_start(g_uart_perf_timer, SLE_UART_PERF_TX_TIMER_1S, at_send_pkt, cycle_num);
        } else {
            uapi_timer_start(g_uart_perf_timer, SLE_UART_PERF_TX_TIMER_US, at_send_pkt, cycle_num);
        }
    }
}

static int start_send_pkt(int argc, char *argv[])
{
    uint32_t pkt_num = 0;

    if (1 != argc) {
        osal_printk("Invalid param input!\r\n");
        return 1;
    }
    pkt_num = (uint32_t)strtol(argv[0], NULL, 0);
    osal_printk("the input argc :%d argv:%s pkt_num:%d\r\n", argc, argv[0], pkt_num);
    if (pkt_send_mode == 0) {
        uapi_timer_start(g_uart_perf_timer, SLE_UART_PERF_TX_TIMER_1S, at_send_pkt, (uintptr_t)pkt_num);
    } else {
        uapi_timer_start(g_uart_perf_timer, SLE_UART_PERF_TX_TIMER_US, at_send_pkt, (uintptr_t)pkt_num);
    }
    return 0;
}
#endif

static void *sle_uart_server_task(const char *arg)
{
    unused(arg);
    uint8_t rx_buf[SLE_UART_SERVER_MSG_QUEUE_MAX_SIZE] = {0};
    uint32_t rx_length = SLE_UART_SERVER_MSG_QUEUE_MAX_SIZE;
    uint8_t sle_connect_state[] = "sle_dis_connect";
    errcode_t ret;

    sle_uart_server_create_msgqueue();
    sle_uart_server_register_msg(sle_uart_server_write_msgqueue);
    sle_uart_server_init(ssaps_server_read_request_cbk, ssaps_server_write_request_cbk);
#ifdef CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE
    sle_uart_low_latency_tx_cbk_register();
    sle_uart_server_low_latency_recv_cbk_register();
#endif
    sle_uart_server_adv_init();
#ifdef SLE_UART_PACK_TEST_MODE
    uapi_test_suite_add_function("pkt_len_change", "<pkt_len_change>", pkt_len_change);
    uapi_test_suite_add_function("pkt_send_mode_change", "<pkt_send_mode_change>", pkt_send_mode_change);
    uapi_test_suite_add_function("start_send_pkt", "<start_send_pkt>", start_send_pkt);
    uart_perf_timer_init();
#else
    ret = uapi_uart_register_rx_callback(CONFIG_SLE_UART_BUS,
                                                   UART_RX_CONDITION_FULL_OR_SUFFICIENT_DATA_OR_IDLE,
                                                   SLE_UART_RX_TRIGGER_LEN, sle_uart_server_read_int_handler);
    if (ret != ERRCODE_SUCC) {
        osal_printk("%s Register uart callback fail.[%x]\r\n", SLE_UART_SERVER_LOG, ret);
        return NULL;
    }
#endif
    while (1) {
        sle_uart_server_rx_buf_init(rx_buf, &rx_length);
        sle_uart_server_receive_msgqueue(rx_buf, &rx_length);
        if (strncmp((const char *)rx_buf, (const char *)sle_connect_state, sizeof(sle_connect_state)) == 0) {
            ret = sle_start_announce(SLE_ADV_HANDLE_DEFAULT);
            if (ret != ERRCODE_SLE_SUCCESS) {
                osal_printk("%s sle_connect_state_changed_cbk,sle_start_announce fail :%02x\r\n",
                    SLE_UART_SERVER_LOG, ret);
            }
        }
        osal_msleep(SLE_UART_TASK_DURATION_MS);
    }
    sle_uart_server_delete_msgqueue();
    return NULL;
}
#elif defined(CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT)
#ifdef SLE_UART_PACK_TEST_MODE
static uint64_t pre_time = 0;
static uint64_t now_time = 0;
static uint64_t pre_num = 0;
static uint64_t now_num = 0;
static uint32_t rcv_err_cnt = 0;
static uint32_t rcv_cnt = 0;
static uint32_t los_cnt = 0;
#define UART_PRINK_CNT   1000
static uint64_t conn_id_delay[UART_PRINK_CNT] = {0};
static uint64_t g_recv_pkt_cnt[UART_PRINK_CNT] = { 0 };
static uint8_t pkt_receive_mode = 0; //0:普通模式；1：测试掉包率模式；2：校验接收数据模式
#define SLE_UART_CLIENT_LOG "[sle uart client]"
#endif

void sle_uart_notification_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data,
    errcode_t status)
{
    unused(client_id);
    unused(conn_id);
    unused(status);
#ifdef SLE_UART_PACK_TEST_MODE
    uint32_t ptr = 0;
    uint64_t trace_data = 0;
    uint8_t packet_len = sizeof(sle_uart_send_buff_test);
    if (data == NULL || data->data == NULL) {
        return;
    }
    if (data->data[9] == 0) {
        pkt_receive_mode = 1;   //测试掉包率模式
    } else if (data->data[9] == 9) {
        pkt_receive_mode = 2;   //测试接收数据校验模式
    }
    if (pkt_receive_mode == 2) {
        // osal_printk("\r\n sle uart ressaps write request callback cbkcived data :");
        for (uint8_t i = 0; i<packet_len;i++) {
            osal_printk(" %d", data->data[i]);
        }
        osal_printk("\r\n");
        if (memcmp(&sle_uart_send_buff_test, data->data, packet_len) != 0) {
            osal_printk("\r\n receive data ERROR\r\n");
            rcv_err_cnt++;
        } else {
            osal_printk("\r\n receive data correct\r\n");
        }
    } else if (pkt_receive_mode == 0) { //普通模式
        // osal_printk("sle uart recived data : %s\r\n", data->data);
        uapi_uart_write(CONFIG_SLE_UART_BUS, (uint8_t *)(data->data), data->data_len, 0);
        return;
    } 

    uapi_gpio_toggle(SLE_UART_S_MGPIO);
    ptr = rcv_cnt;
    memcpy_s(&trace_data, sizeof(uint64_t), data->data, sizeof(uint64_t));

    if (0 == ptr) {
        conn_id_delay[0] = 0;
        pre_time = uapi_tcxo_get_us();
        pre_num = trace_data;
    } else {
        now_time = uapi_tcxo_get_us();
        conn_id_delay[ptr] = now_time - pre_time;
        pre_time = now_time;
        now_num = trace_data;
        if (now_num != (pre_num + 1)) {
            los_cnt += now_num - pre_num - 1;
        }
        pre_num = now_num;
    }
    g_recv_pkt_cnt[ptr] = trace_data;
    rcv_cnt++;
#else
    // osal_printk("\n sle uart recived data : %s\r\n", data->data);
    uapi_uart_write(CONFIG_SLE_UART_BUS, (uint8_t *)(data->data), data->data_len, 0);
#endif
}

void sle_uart_indication_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data,
    errcode_t status)
{
    unused(client_id);
    unused(conn_id);
    unused(status);
    // osal_printk("\n sle uart recived data : %s\r\n", data->data);
    uapi_uart_write(CONFIG_SLE_UART_BUS, (uint8_t *)(data->data), data->data_len, 0);
}
    
#ifdef SLE_UART_PACK_TEST_MODE
static int dump_rcv_stat(int argc, char *argv[])
{
    unused(argv);
    uint32_t i = 0;
    if (0 != argc) {
        osal_printk("%s This AT command input error!\r\n",SLE_UART_CLIENT_LOG);
       return 1;
    }
    if(pkt_receive_mode == 1) {
        osal_printk("===========================\r\n los_cnt:%04d rcv_cnt:%04d\r\n", los_cnt, rcv_cnt);
    } else {
        osal_printk("===========================\r\n rcv_cnt:%04d rcv_error_cnt:%04d\r\n", rcv_cnt, rcv_err_cnt);
    }
    for (i = 0 ; i < rcv_cnt; i++)
    {
        //osal_printk("rx pkt num [%04d]: %04d  delay: %lluus\r\n", i, g_recv_pkt_cnt[i], conn_id_delay[i]);
        conn_id_delay[i] = 0;
        g_recv_pkt_cnt[i] = 0;
    }
    osal_printk("===========================\r\n");
    rcv_cnt = 0;
    los_cnt = 0;
    rcv_err_cnt = 0;
    return 0;
}
#else
#ifdef CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE
static uint8_t g_sle_uart_client_low_latency_tx_buf[SLE_UART_AGGREGATE_MAX_LEN] = {0};
static uint16_t g_sle_uart_client_low_latency_tx_len = 0;
static uint8_t g_sle_uart_client_low_latency_tx_valid = 0;

static uint8_t *sle_uart_client_low_latency_tx_cbk(uint16_t *len)
{
    if ((len == NULL) || (g_sle_uart_client_low_latency_tx_valid == 0) || (g_sle_uart_client_low_latency_tx_len == 0)) {
        return NULL;
    }
    *len = g_sle_uart_client_low_latency_tx_len;
    g_sle_uart_client_low_latency_tx_valid = 0;
    g_sle_uart_client_low_latency_tx_len = 0;
    return g_sle_uart_client_low_latency_tx_buf;
}

static void sle_uart_client_low_latency_tx_cbk_register(void)
{
    sle_low_latency_tx_callbacks_t cbk_func = {0};
    cbk_func.low_latency_tx_cb = sle_uart_client_low_latency_tx_cbk;
    sle_low_latency_tx_register_callbacks(&cbk_func);
}
#endif

static void sle_uart_client_read_int_handler(const void *buffer, uint16_t length, bool error)
{
    unused(error);
#ifdef CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE
    uint16_t copy_len;
    uint16_t remain_len;
    if ((buffer == NULL) || (length == 0)) {
        return;
    }
    if (g_sle_uart_client_low_latency_tx_valid == 0) {
        g_sle_uart_client_low_latency_tx_len = 0;
    }
    remain_len = SLE_UART_AGGREGATE_MAX_LEN - g_sle_uart_client_low_latency_tx_len;
    copy_len = (length > remain_len) ? remain_len : length;
    if (copy_len == 0) {
        return;
    }
    if (memcpy_s(g_sle_uart_client_low_latency_tx_buf + g_sle_uart_client_low_latency_tx_len,
        sizeof(g_sle_uart_client_low_latency_tx_buf) - g_sle_uart_client_low_latency_tx_len, buffer, copy_len) != EOK) {
        return;
    }
    g_sle_uart_client_low_latency_tx_len += copy_len;
    g_sle_uart_client_low_latency_tx_valid = 1;
#else
    ssapc_write_param_t *sle_uart_send_param = get_g_sle_uart_send_param();
    uint16_t g_sle_uart_conn_id = get_g_sle_uart_conn_id();
    sle_uart_send_param->data_len = length;
    sle_uart_send_param->data = (uint8_t *)buffer;
    ssapc_write_req(0, g_sle_uart_conn_id, sle_uart_send_param);
#endif
}
#endif  /*SLE_UART_PACK_TEST_MODE*/

static void *sle_uart_client_task(const char *arg)
{
    unused(arg);
#ifdef SLE_UART_PACK_TEST_MODE
    uapi_test_suite_add_function("dump_rcv_stat", "<dump_rcv_stat>", dump_rcv_stat);
#else
#ifdef CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE
    sle_uart_client_low_latency_tx_cbk_register();
#endif
    errcode_t ret = uapi_uart_register_rx_callback(CONFIG_SLE_UART_BUS,
                                                   UART_RX_CONDITION_FULL_OR_SUFFICIENT_DATA_OR_IDLE,
                                                   SLE_UART_RX_TRIGGER_LEN, sle_uart_client_read_int_handler);
    if (ret != ERRCODE_SUCC) {
        osal_printk("Register uart callback fail.");
        return NULL;
    }
#endif  /*SLE_UART_PACK_TEST_MODE*/
    return NULL;
}
#endif  /* CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT */

static void sle_uart_entry(void)
{
    osal_task *task_handle = NULL;
    if (uapi_clock_control(CLOCK_CONTROL_FREQ_LEVEL_CONFIG, CLOCK_FREQ_LEVEL_HIGH) == ERRCODE_SUCC) {
        osal_printk("Clock config succ.\r\n");
    } else {
        osal_printk("Clock config fail.\r\n");
    }
    osal_kthread_lock();
#if defined(CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER)
    sle_device_register_cbks();
    task_handle = osal_kthread_create((osal_kthread_handler)sle_uart_server_task, 0, "SLEUartServerTask",
                                      SLE_UART_TASK_STACK_SIZE);
#elif defined(CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT)
    sle_uart_client_sample_dev_cbk_register();
    task_handle = osal_kthread_create((osal_kthread_handler)sle_uart_client_task, 0, "SLEUartDongleTask",
                                      SLE_UART_TASK_STACK_SIZE);
#endif /* CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT */
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, SLE_UART_TASK_PRIO);
    }
    osal_kthread_unlock();
}

/* Run the sle_uart_entry. */
app_run(sle_uart_entry);
