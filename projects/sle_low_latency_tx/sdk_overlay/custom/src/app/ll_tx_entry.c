#include "ll_common.h"
#include "app_init.h"
#include "osal_debug.h"
#include "soc_osal.h"

static int ll_tx_task(void *data)
{
    (void)data;
    ll_mark_init(LL_TX_MARK_GPIO);
    osal_msleep(2500);
    ll_tx_init();
    while (1) {
        ll_tx_poll();
        osal_msleep(1);
    }
    return 0;
}

static void ll_tx_overlay_entry(void)
{
    osal_task *task = osal_kthread_create(ll_tx_task, NULL, "llTxTask", 0x800);
    if (task != NULL) {
        (void)osal_kthread_set_priority(task, 26);
    }
    osal_printk("[ll_tx] overlay boot\n");
}

void of_sle_on_link_state(int connected)
{
    ll_tx_on_link_state(connected);
}

app_run(ll_tx_overlay_entry);
