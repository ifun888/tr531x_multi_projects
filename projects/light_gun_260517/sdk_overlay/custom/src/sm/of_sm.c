#include "of_sm.h"
#include "of_selftest.h"
#include "of_protocol.h"

static of_boot_state_t g_boot = OF_BOOT_EARLY;
static of_mode_t g_mode = OF_MODE_RUN;
static of_event_t g_evt = EVT_NONE;

void of_sm_init(void)
{
    g_boot = OF_BOOT_EARLY;
    g_mode = OF_MODE_RUN;
    g_evt = EVT_NONE;
    of_proto_set_mode(g_mode);
}

void of_sm_post_event(of_event_t evt)
{
    g_evt = evt;
}

void of_sm_step(void)
{
    if (g_evt != EVT_NONE) {
        switch (g_evt) {
            case EVT_MODE_REQ_RUN:
                g_mode = OF_MODE_RUN;
                break;
            case EVT_MODE_REQ_PAUSE:
                g_mode = OF_MODE_PAUSE;
                break;
            case EVT_MODE_REQ_CALIB:
                g_mode = OF_MODE_CALIB;
                break;
            case EVT_MODE_REQ_DOCKED:
                g_mode = OF_MODE_DOCKED;
                break;
            case EVT_MODE_REQ_FAULT:
            case EVT_SELFTEST_FAIL:
                g_mode = OF_MODE_FAULT;
                break;
            default:
                break;
        }
        of_proto_set_mode(g_mode);
        g_evt = EVT_NONE;
    }

    switch (g_boot) {
        case OF_BOOT_EARLY:
            g_boot = OF_BOOT_DRIVER_INIT;
            break;
        case OF_BOOT_DRIVER_INIT:
            g_boot = OF_BOOT_CONFIG_LOAD;
            break;
        case OF_BOOT_CONFIG_LOAD:
            g_boot = OF_BOOT_SELFTEST;
            break;
        case OF_BOOT_SELFTEST:
            of_selftest_run();
            g_boot = of_selftest_all_critical_pass() ? OF_BOOT_LINK_SELECT : OF_BOOT_FAIL;
            break;
        case OF_BOOT_LINK_SELECT:
            g_boot = OF_BOOT_READY;
            of_sm_post_event(EVT_BOOT_DONE);
            break;
        default:
            break;
    }
}

of_boot_state_t of_sm_get_boot_state(void)
{
    return g_boot;
}

of_mode_t of_sm_get_mode(void)
{
    return g_mode;
}
