#include <stdint.h>
#include "drivers/drv_sle_link.h"
#include "osal_debug.h"

void of_sle_on_rx_data(const uint8_t *data, uint16_t len)
{
    drv_sle_link_push_rx(data, (uint32_t)len);
}

void of_sle_on_link_state(int connected)
{
    osal_printk("[openfire][dongle] sdk link state=%d\n", connected);
    drv_sle_link_set_connected(connected);
}
