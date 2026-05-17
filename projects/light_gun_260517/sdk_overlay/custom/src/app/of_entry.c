#include "osal_debug.h"
#include "soc_osal.h"
#include "of_sm.h"
#include "of_transport.h"
#include "of_diag.h"

int svc_transport_route_auto(void);
int svc_transport_route_tick(void);
void of_runtime_once(void);
static int g_route_task_started = 0;

static int of_route_task(void *data)
{
    (void)data;
    while (1) {
        (void)svc_transport_route_tick();
        osal_msleep(2);
    }
    return 0;
}

void demo_sle_uart_overlay_entry(void)
{
    int i;
    of_sm_init();
    for (i = 0; i < 5; i++) {
        of_sm_step();
    }

    of_diag_init();
    if (svc_transport_route_auto() != 0) {
        (void)of_transport_init(OF_TRANSPORT_USB_CDC);
    }
    if (!g_route_task_started) {
        osal_task *task = osal_kthread_create(of_route_task, 0, "ofRoute", 0x600);
        if (task != 0) {
            (void)osal_kthread_set_priority(task, 24);
            g_route_task_started = 1;
        }
    }

    for (i = 0; i < 8; i++) {
        of_runtime_once();
    }

    osal_printk("[openfire-tr531x] M5 skeleton boot=%d link=%d\n",
        (int)of_sm_get_boot_state(), (int)of_transport_get_type());
}
