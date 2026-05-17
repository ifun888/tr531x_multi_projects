#include "of_diag.h"
#include "osal_debug.h"

#define DIAG_WIN 128U

static uint32_t g_tx = 0;
static uint32_t g_rx = 0;
static uint32_t g_tick = 0;
static uint32_t g_lat[DIAG_WIN];
static uint32_t g_lat_idx = 0;
static uint32_t g_lat_cnt = 0;

static void diag_sort_u32(uint32_t *a, uint32_t n)
{
    uint32_t i;
    uint32_t j;
    for (i = 0; i < n; i++) {
        for (j = i + 1U; j < n; j++) {
            if (a[j] < a[i]) {
                uint32_t t = a[i];
                a[i] = a[j];
                a[j] = t;
            }
        }
    }
}

void of_diag_init(void)
{
    uint32_t i;
    g_tx = 0;
    g_rx = 0;
    g_tick = 0;
    g_lat_idx = 0;
    for (i = 0; i < DIAG_WIN; i++) {
        g_lat[i] = 0U;
    }
}

void of_diag_on_tx(uint32_t bytes)
{
    g_tx += bytes;
}

void of_diag_on_rx(uint32_t bytes)
{
    g_rx += bytes;
}

void of_diag_on_rtt_us(uint32_t us)
{
    g_lat[g_lat_idx] = us;
    g_lat_idx = (g_lat_idx + 1U) % DIAG_WIN;
    if (g_lat_cnt < DIAG_WIN) {
        g_lat_cnt++;
    }
}

void of_diag_tick(void)
{
    uint32_t sample_cnt;
    uint32_t tmp[DIAG_WIN];
    uint32_t i;
    uint32_t p50;
    uint32_t p95;
    uint32_t p99;

    g_tick++;
    if ((g_tick % 100U) != 0U) {
        return;
    }

    sample_cnt = g_lat_cnt;
    if (sample_cnt == 0U) {
        osal_printk("[openfire-diag] tx=%u rx=%u rtt_us p50=0 p95=0 p99=0 n=0\n", g_tx, g_rx);
        return;
    }

    for (i = 0; i < sample_cnt; i++) {
        tmp[i] = g_lat[i];
    }
    diag_sort_u32(tmp, sample_cnt);

    p50 = tmp[(sample_cnt * 50U) / 100U];
    p95 = tmp[(sample_cnt * 95U) / 100U];
    p99 = tmp[(sample_cnt * 99U) / 100U];

    osal_printk("[openfire-diag] tx=%u rx=%u rtt_us p50=%u p95=%u p99=%u n=%u\n",
        g_tx, g_rx, p50, p95, p99, sample_cnt);
}
