#include "ll_common.h"
#include "app_init.h"
#include "osal_debug.h"
#include "soc_osal.h"

static int ll_rx_task(void *data)
{
    (void)data;
    ll_mark_init(LL_RX_MARK_GPIO);
    osal_msleep(2500);
    ll_rx_init();
    while (1) {
        ll_rx_poll();
        osal_msleep(1);
    }
    return 0;
}

static void ll_rx_overlay_entry(void)
{
    osal_task *task = osal_kthread_create(ll_rx_task, NULL, "llRxTask", 0x800);
    if (task != NULL) {
        (void)osal_kthread_set_priority(task, 26);
    }
    osal_printk("[ll_rx] overlay boot\n");
}

app_run(ll_rx_overlay_entry);
