#include <stdint.h>
#include "drivers/drv_sle_link.h"
#include "osal_debug.h"
#include "sle_ssap_client.h"

void of_sle_on_rx_data(const uint8_t *data, uint16_t len)
{
    drv_sle_link_push_rx(data, (uint32_t)len);
}

void of_sle_on_link_state(int connected)
{
    osal_printk("[openfire][gun] sdk link state=%d\n", connected);
    drv_sle_link_set_connected(connected);
}

void sle_uart_notification_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    (void)client_id;
    (void)conn_id;
    (void)status;
    if ((data != 0) && (data->data != 0) && (data->data_len > 0U)) {
        of_sle_on_rx_data((const uint8_t *)data->data, data->data_len);
    }
}

void sle_uart_indication_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    (void)client_id;
    (void)conn_id;
    (void)status;
    if ((data != 0) && (data->data != 0) && (data->data_len > 0U)) {
        of_sle_on_rx_data((const uint8_t *)data->data, data->data_len);
    }
}
