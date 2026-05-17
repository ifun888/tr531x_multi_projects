#include <stdint.h>

extern uint16_t sle_uart_client_is_connected(void);
extern int sle_uart_server_send_report_by_handle(const uint8_t *data, uint8_t len);

/*
 * Dongle端：复用 sle_uart server 的 notify path，向已连接对端发送串口语义数据。
 * 返回0表示发送请求提交成功。
 */
int of_sle_sdk_send(const uint8_t *buf, uint32_t len)
{
    if ((buf == 0) || (len == 0U) || (len > 255U)) {
        return -1;
    }

    if (sle_uart_client_is_connected() == 0U) {
        return -1;
    }

    if (sle_uart_server_send_report_by_handle(buf, (uint8_t)len) != 0) {
        return -1;
    }

    return 0;
}
