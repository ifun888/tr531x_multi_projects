#include "of_transport.h"
#include "of_protocol.h"
#include "of_diag.h"
#include <stdint.h>

#ifndef OF_ROLE_DONGLE
#define OF_ROLE_DONGLE 0
#endif
#ifndef OF_ENABLE_HEARTBEAT
#define OF_ENABLE_HEARTBEAT 0
#endif

#define OF_STRESS_TYPE_PING 1U
#define OF_STRESS_TYPE_PONG 2U
#define OF_STRESS_HDR_SZ 9U

static int of_try_handle_stress_ping(const uint8_t *rx, uint32_t got)
{
    uint8_t tx[80];
    uint32_t sent = 0;
    uint32_t i;

    if ((rx == 0) || (got < OF_STRESS_HDR_SZ)) {
        return 0;
    }
    if ((rx[0] != 'O') || (rx[1] != 'F') || (rx[2] != OF_STRESS_TYPE_PING)) {
        return 0;
    }

    if (got > sizeof(tx)) {
        got = sizeof(tx);
    }
    for (i = 0; i < got; i++) {
        tx[i] = rx[i];
    }
    tx[2] = OF_STRESS_TYPE_PONG;
    (void)of_transport_write(tx, got, &sent);
    of_diag_on_tx(sent);
    return 1;
}

void of_runtime_once(void)
{
    extern uint64_t uapi_tcxo_get_us(void) __attribute__((weak));
    int svc_transport_route_tick(void);
    uint8_t rx[64] = {0};
    uint32_t got = 0;
    uint64_t t0 = 0;
    uint64_t t1 = 0;
    static const uint8_t hb_gun[] = {'G','U','N','\n'};
    static const uint8_t hb_dongle[] = {'D','N','G','\n'};

    (void)svc_transport_route_tick();

    if (of_transport_read(rx, sizeof(rx), &got) == 0 && got > 0) {
        of_diag_on_rx(got);
        if (!of_try_handle_stress_ping(rx, got)) {
            of_proto_docked_process(rx, got);
            of_proto_mh_process(rx, got);
        }
    }

    if (OF_ENABLE_HEARTBEAT) {
        uint32_t sent = 0;
        t0 = (uapi_tcxo_get_us != 0) ? uapi_tcxo_get_us() : 0;
        if (OF_ROLE_DONGLE) {
            (void)of_transport_write(hb_dongle, sizeof(hb_dongle), &sent);
        } else {
            (void)of_transport_write(hb_gun, sizeof(hb_gun), &sent);
        }
        t1 = (uapi_tcxo_get_us != 0) ? uapi_tcxo_get_us() : 0;
        of_diag_on_tx(sent);
        if (t1 > t0) {
            of_diag_on_rtt_us((uint32_t)(t1 - t0));
        }
    }

    of_diag_tick();
}
