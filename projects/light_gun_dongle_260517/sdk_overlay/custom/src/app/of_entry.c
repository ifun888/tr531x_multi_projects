#include "osal_debug.h"
#include "soc_osal.h"
#include "of_sm.h"
#include "of_transport.h"
#include "drivers/drv_sle_link.h"
#include "drivers/drv_usb_cdc.h"
#include "drivers/drv_usb_hid.h"
#include "of_diag.h"
#include "drivers/drv_storage.h"
#include "services/svc_profile.h"
#include "services/svc_binding.h"
#include "services/svc_calibration.h"

void of_runtime_once(void);

static void of_init_peripherals(void)
{
    const of_dev_t *dev = drv_storage_get_dev();
    if ((dev != 0) && (dev->ops != 0) && (dev->ops->open != 0)) {
        (void)dev->ops->open(dev->priv);
    }
}

void demo_sle_uart_overlay_entry(void)
{
    int i;
    const of_dev_t *usb = drv_usb_cdc_get_dev();
    of_sm_init();
    of_init_peripherals();
    (void)svc_profile_load();
    (void)svc_binding_load();
    (void)svc_calibration_exit();
    (void)svc_calibration_load_profile();
    for (i = 0; i < 5; i++) {
        of_sm_step();
    }

    of_diag_init();
    (void)of_transport_init(OF_TRANSPORT_SLE);
    if ((usb != 0) && (usb->ops != 0) && (usb->ops->open != 0)) {
        (void)usb->ops->open(usb->priv);
    }
    if (drv_usb_hid_init() == 0) {
        drv_usb_hid_set_ready(1);
        osal_printk("[openfire-tr531x] dongle usb hid ready.\r\n");
    }

    for (i = 0; i < 8; i++) {
        of_runtime_once();
    }

    osal_printk("[openfire-tr531x] M5 skeleton boot=%d link=%d\n",
        (int)of_sm_get_boot_state(), (int)of_transport_get_type());
}
