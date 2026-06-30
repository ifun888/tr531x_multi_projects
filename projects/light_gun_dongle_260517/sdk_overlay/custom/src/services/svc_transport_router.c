#include "of_transport.h"
#include "of_types.h"

static int svc_route_switch(of_transport_type_t type)
{
    static of_transport_type_t g_cur_type = OF_TRANSPORT_USB_CDC;
    static int g_route_ready = 0;

    if (g_route_ready && (g_cur_type == type)) {
        return 0;
    }
    if (g_route_ready) {
        (void)of_transport_deinit();
    }
    if (of_transport_init(type) == 0) {
        g_cur_type = type;
        g_route_ready = 1;
        return 0;
    }
    g_route_ready = 0;
    return -1;
}

int svc_transport_route_init(of_link_type_t link)
{
    (void)link;
    return svc_route_switch(OF_TRANSPORT_SLE);
}

int svc_transport_route_auto(void)
{
    return svc_route_switch(OF_TRANSPORT_SLE);
}

int svc_transport_route_tick(void)
{
    return svc_route_switch(OF_TRANSPORT_SLE);
}
