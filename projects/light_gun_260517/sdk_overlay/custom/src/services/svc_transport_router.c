#include "of_transport.h"
#include "of_types.h"
#include "of_link_io.h"
#include "drivers/drv_usb_cdc.h"
#include "drivers/drv_sle_link.h"

static of_transport_type_t g_cur_type = OF_TRANSPORT_USB_CDC;
static int g_route_ready = 0;
static uint32_t g_usb_stable = 0;
static uint32_t g_sle_stable = 0;
static uint32_t g_hold_ticks = 0;
static uint32_t g_switch_cooldown = 0;

static int svc_route_switch(of_transport_type_t type)
{
    if (g_route_ready && (g_cur_type == type)) {
        g_hold_ticks++;
        return 0;
    }
    if (g_switch_cooldown > 0U) {
        return 0;
    }
    if (g_hold_ticks < 20U && g_route_ready) {
        return 0;
    }
    if (g_route_ready) {
        (void)of_transport_deinit();
    }
    if (of_transport_init(type) == 0) {
        g_cur_type = type;
        g_route_ready = 1;
        g_hold_ticks = 0;
        g_switch_cooldown = 30;
        return 0;
    }
    g_route_ready = 0;
    return -1;
}

int svc_transport_route_init(of_link_type_t link)
{
    if (link == OF_LINK_USB_CDC) {
        return svc_route_switch(OF_TRANSPORT_USB_CDC);
    }
    return svc_route_switch(OF_TRANSPORT_SLE);
}

int svc_transport_route_auto(void)
{
    if (of_link_is_ready()) {
        return svc_route_switch(OF_TRANSPORT_SLE);
    }
    if (drv_usb_cdc_is_ready()) {
        return svc_route_switch(OF_TRANSPORT_USB_CDC);
    }
    return -1;
}

int svc_transport_route_tick(void)
{
    int usb_ready = drv_usb_cdc_is_ready();
    int sle_ready = of_link_is_ready();
    const uint32_t stable_th = 5U;
    if (g_switch_cooldown > 0U) {
        g_switch_cooldown--;
    }

    g_usb_stable = usb_ready ? (g_usb_stable + 1U) : 0U;
    g_sle_stable = sle_ready ? (g_sle_stable + 1U) : 0U;

    if (g_sle_stable >= stable_th) {
        return svc_route_switch(OF_TRANSPORT_SLE);
    }
    if (g_usb_stable >= stable_th) {
        return svc_route_switch(OF_TRANSPORT_USB_CDC);
    }
    return 0;
}
