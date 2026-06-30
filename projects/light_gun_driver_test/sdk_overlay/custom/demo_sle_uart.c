#include "app_init.h"
#include "osal_debug.h"
#include "ir_test.h"
#include "rumble_test.h"
#include "solenoid_test.h"
#include "usb_sim_test.h"

void demo_sle_uart_overlay_entry(void)
{
    osal_printk("[light_gun_driver_test] overlay boot.\n");
#if defined(CONFIG_LIGHT_GUN_TEST_ENABLE_SOLENOID)
    solenoid_test_overlay_entry();
#endif
#if defined(CONFIG_LIGHT_GUN_TEST_ENABLE_RUMBLE)
    rumble_test_overlay_entry();
#endif
#if defined(CONFIG_LIGHT_GUN_TEST_ENABLE_IR)
    ir_test_overlay_entry();
#endif
#if defined(CONFIG_LIGHT_GUN_TEST_ENABLE_USB_SIM)
    usb_sim_test_overlay_entry();
#endif
}

app_run(demo_sle_uart_overlay_entry);
