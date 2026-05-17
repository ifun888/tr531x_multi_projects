#include "osal_debug.h"
#include "soc_osal.h"
#include "of_sm.h"
#include "of_transport.h"
#include "of_diag.h"
#include "drivers/drv_ir_cam.h"
#include "drivers/drv_input_keys.h"
#include "drivers/drv_input_adc.h"
#include "drivers/drv_feedback.h"
#include "drivers/drv_led.h"
#include "drivers/drv_storage.h"
#include "drivers/drv_temp_sensor.h"
#include "services/svc_profile.h"
#include "services/svc_binding.h"
#include "services/svc_calibration.h"

int svc_transport_route_auto(void);
int svc_transport_route_tick(void);
void of_runtime_once(void);
static int g_route_task_started = 0;

static void of_init_peripherals(void)
{
    const of_dev_t *devs[] = {
        drv_storage_get_dev(),
        drv_led_get_dev(),
        drv_ir_cam_get_dev(),
        drv_input_keys_get_dev(),
        drv_input_adc_get_dev(),
        drv_feedback_get_dev(),
        drv_temp_sensor_get_dev(),
    };
    unsigned int i;
    for (i = 0; i < (sizeof(devs) / sizeof(devs[0])); i++) {
        if ((devs[i] != 0) && (devs[i]->ops != 0) && (devs[i]->ops->open != 0)) {
            (void)devs[i]->ops->open(devs[i]->priv);
        }
    }
}

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
    of_init_peripherals();
    (void)svc_profile_load();
    (void)svc_binding_load();
    (void)svc_calibration_exit();
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
