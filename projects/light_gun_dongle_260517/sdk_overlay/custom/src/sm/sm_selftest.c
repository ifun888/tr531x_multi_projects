#include "of_selftest.h"
#include "of_link_io.h"
#include "drivers/drv_usb_cdc.h"

static of_selftest_result_t g_result[OF_ST_MAX];
static uint32_t g_bitmap = 0;

void of_selftest_run(void)
{
    int i;
    g_bitmap = 0;
    for (i = 0; i < OF_ST_MAX; i++) {
        g_result[i] = OF_ST_RES_PASS;
        g_bitmap |= (1U << i);
    }

    if (!drv_usb_cdc_is_ready()) {
        g_result[OF_ST_USB] = OF_ST_RES_WARN;
    }
    if (!of_link_is_ready()) {
        g_result[OF_ST_SLE] = OF_ST_RES_WARN;
    }
}

of_selftest_result_t of_selftest_get(of_selftest_item_t item)
{
    if (item >= OF_ST_MAX) {
        return OF_ST_RES_FAIL;
    }
    return g_result[item];
}

int of_selftest_all_critical_pass(void)
{
    if (g_result[OF_ST_IR] == OF_ST_RES_FAIL) return 0;
    if (g_result[OF_ST_INPUT] == OF_ST_RES_FAIL) return 0;
    if (g_result[OF_ST_STORAGE] == OF_ST_RES_FAIL) return 0;
    return 1;
}

uint32_t of_selftest_bitmap(void)
{
    return g_bitmap;
}
