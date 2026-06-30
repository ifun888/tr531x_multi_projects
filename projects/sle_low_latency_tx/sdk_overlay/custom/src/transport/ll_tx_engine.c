#include "ll_common.h"
#include "osal_debug.h"
#include "securec.h"
#include "sle_low_latency.h"
#include "tcxo.h"

static sle_low_latency_tx_callbacks_t g_tx_cbk;
static uint8_t g_tx_buf[50];
static uint16_t g_tx_len = LL_DEFAULT_PKT_LEN;
static volatile uint8_t g_tx_valid;
static volatile uint8_t g_tx_connected;
static uint32_t g_tx_seq;
static uint64_t g_next_send_us;

static void ll_fill_packet(void)
{
    if (g_tx_len < 8U) {
        g_tx_len = 8U;
    }
    g_tx_buf[0] = LL_TEST_MAGIC0;
    g_tx_buf[1] = LL_TEST_MAGIC1;
    g_tx_buf[2] = LL_TEST_VERSION;
    g_tx_buf[3] = (uint8_t)g_tx_len;
    g_tx_buf[4] = (uint8_t)(g_tx_seq & 0xFFU);
    g_tx_buf[5] = (uint8_t)((g_tx_seq >> 8) & 0xFFU);
    g_tx_buf[6] = (uint8_t)((g_tx_seq >> 16) & 0xFFU);
    g_tx_buf[7] = (uint8_t)((g_tx_seq >> 24) & 0xFFU);
    if (g_tx_len > 8U) {
        (void)memset_s(&g_tx_buf[8], sizeof(g_tx_buf) - 8U, 0xA5, g_tx_len - 8U);
    }
    g_tx_seq++;
}

static uint8_t *ll_low_latency_tx_cb(uint16_t *len)
{
    if ((len == NULL) || (g_tx_valid == 0U)) {
        return NULL;
    }
    *len = g_tx_len;
    g_tx_valid = 0;
    return g_tx_buf;
}

void ll_tx_init(void)
{
    g_tx_cbk.low_latency_tx_cb = ll_low_latency_tx_cb;
    (void)sle_low_latency_tx_register_callbacks(&g_tx_cbk);
    g_next_send_us = uapi_tcxo_get_us() + (uint64_t)LL_DEFAULT_INTERVAL_MS * 1000ULL;
    osal_printk("[ll_tx] low latency tx callback registered\n");
}

void ll_tx_on_link_state(int connected)
{
    g_tx_connected = (connected != 0) ? 1U : 0U;
    if (g_tx_connected != 0U) {
        g_next_send_us = uapi_tcxo_get_us() + (uint64_t)LL_DEFAULT_INTERVAL_MS * 1000ULL;
    }
    osal_printk("[ll_tx] link=%u\n", g_tx_connected);
}

void ll_tx_poll(void)
{
    uint64_t now_us;

    ll_mark_poll(LL_TX_MARK_GPIO);
    if ((g_tx_connected == 0U) || (g_tx_valid != 0U)) {
        return;
    }

    now_us = uapi_tcxo_get_us();
    if (now_us < g_next_send_us) {
        return;
    }

    ll_fill_packet();
    ll_mark_pulse_start(LL_TX_MARK_GPIO);
    g_tx_valid = 1;
    g_next_send_us = now_us + (uint64_t)LL_DEFAULT_INTERVAL_MS * 1000ULL;
}
