#include "ll_common.h"
#include "osal_debug.h"
#include "sle_low_latency.h"
#include <stddef.h>

static sle_low_latency_rx_callbacks_t g_rx_cbk;
static uint32_t g_rx_count;
static uint32_t g_last_seq;
static uint32_t g_rx_drop;

static void ll_low_latency_rx_cb(uint16_t len, uint8_t *value)
{
    ll_rx_process_packet(value, len);
}

void ll_rx_process_packet(const uint8_t *data, uint16_t len)
{
    uint32_t seq;

    if ((data == NULL) || (len < 8U)) {
        return;
    }
    if ((data[0] != LL_TEST_MAGIC0) || (data[1] != LL_TEST_MAGIC1) || (data[2] != LL_TEST_VERSION)) {
        return;
    }

    seq = ((uint32_t)data[4]) |
        ((uint32_t)data[5] << 8) |
        ((uint32_t)data[6] << 16) |
        ((uint32_t)data[7] << 24);

    if ((g_rx_count > 0U) && (seq != (g_last_seq + 1U))) {
        g_rx_drop += (seq > g_last_seq) ? (seq - g_last_seq - 1U) : 0U;
    }
    g_last_seq = seq;
    g_rx_count++;
    ll_mark_pulse_start(LL_RX_MARK_GPIO);

    if ((g_rx_count % 100U) == 0U) {
        osal_printk("[ll_rx] count=%u drop=%u last_seq=%u\n", g_rx_count, g_rx_drop, g_last_seq);
    }
}

void ll_rx_init(void)
{
    g_rx_cbk.low_latency_rx_cb = ll_low_latency_rx_cb;
    (void)sle_low_latency_rx_register_callbacks(&g_rx_cbk);
    osal_printk("[ll_rx] low latency rx callback registered\n");
}

void ll_rx_poll(void)
{
    ll_mark_poll(LL_RX_MARK_GPIO);
}

void of_sle_on_rx_data(const uint8_t *data, uint16_t len)
{
    ll_rx_process_packet(data, len);
}

void of_sle_on_link_state(int connected)
{
    osal_printk("[ll_rx] link=%d\n", connected);
}
