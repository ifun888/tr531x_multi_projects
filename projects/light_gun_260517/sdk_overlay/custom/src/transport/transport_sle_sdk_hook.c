#include <stdint.h>
#include "sle_ssap_client.h"
#include "sle_connection_manager.h"

extern ssapc_write_param_t *get_g_sle_uart_send_param(void);
extern uint16_t get_g_sle_uart_conn_id(void);
extern void sle_uart_start_scan(void) __attribute__((weak));

/*
 * 某些构建组合下，SDK 的 sle_uart client 示例源码没有真正把这两个导出符号链接进来。
 * 这里提供弱桩，保证系统至少能先启动起来；如果后续 sample 侧强符号存在，会自动覆盖这里。
 */
__attribute__((weak)) uint16_t get_g_sle_uart_conn_id(void)
{
    return 0U;
}

__attribute__((weak)) ssapc_write_param_t *get_g_sle_uart_send_param(void)
{
    return 0;
}

/*
 * 光枪端：复用 sle_uart client 的 write path，把串口语义数据透传到 SLE。
 * 返回0表示发送请求提交成功。
 */
int of_sle_sdk_send(const uint8_t *buf, uint32_t len)
{
    ssapc_write_param_t *param;

    if ((buf == 0) || (len == 0U)) {
        return -1;
    }

    param = get_g_sle_uart_send_param();
    if (param == 0) {
        return -1;
    }

    param->data = (uint8_t *)(uintptr_t)buf;
    param->data_len = (uint16_t)len;
    if (ssapc_write_cmd(0, get_g_sle_uart_conn_id(), param) != 0) {
        return -1;
    }

    return 0;
}

void of_sle_sdk_enter_search(void)
{
    if (sle_uart_start_scan != 0) {
        sle_uart_start_scan();
    }
}

void of_sle_sdk_force_reconnect(void)
{
    (void)sle_disconnect_all_remote_device();
    if (sle_uart_start_scan != 0) {
        sle_uart_start_scan();
    }
}
