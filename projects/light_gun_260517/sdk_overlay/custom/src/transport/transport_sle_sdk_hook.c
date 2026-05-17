#include <stdint.h>
#include "sle_ssap_client.h"

extern ssapc_write_param_t *get_g_sle_uart_send_param(void);
extern uint16_t get_g_sle_uart_conn_id(void);

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
