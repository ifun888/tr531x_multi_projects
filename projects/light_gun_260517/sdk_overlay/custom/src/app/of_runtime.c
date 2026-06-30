#include "of_transport.h"
#include "of_protocol.h"
#include "of_diag.h"
#include "of_link_io.h"
#include "of_wireless_pkt.h"
#include "drivers/drv_ir_cam.h"
#include "drivers/drv_input_keys.h"
#include "drivers/drv_input_adc.h"
#include "drivers/drv_temp_sensor.h"
#include "drivers/drv_feedback.h"
#include "drivers/drv_led.h"
#include "of_sm.h"
#include "services/svc_binding.h"
#include "services/svc_calibration.h"
#include "services/svc_position.h"
#include "services/svc_usb_hid.h"
#include "osal_debug.h"
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
    (void)of_link_send_serial(tx, got, &sent);
    of_diag_on_tx(sent);
    return 1;
}

static void of_publish_runtime_telemetry(void)
{
    extern uint64_t uapi_tcxo_get_us(void) __attribute__((weak));
    static uint64_t s_last_us = 0;
    static uint8_t s_prev_wireless_ready = 0U;
    uint64_t now_us = (uapi_tcxo_get_us != 0) ? uapi_tcxo_get_us() : 0;
    uint8_t pkt[20] = {0};
    uint8_t keys[2] = {0};
    uint8_t adc[4] = {0};
    uint8_t temp[2] = {0};
    uint32_t n = 0;
    uint32_t sent = 0;
    uint8_t key_map0 = 0;
    uint8_t mapped_keys = 0;
    of_pos_sample_t pos = {0};
    uint8_t wireless_link_up = of_link_wireless_active() ? 1U : 0U;
    uint8_t wireless_ready = of_link_is_ready() ? 1U : 0U;
    const of_dev_t *keys_dev = drv_input_keys_get_dev();
    const of_dev_t *adc_dev = drv_input_adc_get_dev();
    const of_dev_t *temp_dev = drv_temp_sensor_get_dev();

    if (wireless_link_up != 0U) {
        if (wireless_ready == 0U) {
            s_prev_wireless_ready = 0U;
            s_last_us = 0;
            return;
        }
        if (s_prev_wireless_ready == 0U) {
            s_last_us = 0;
        }
    }

    if ((now_us != 0) && (s_last_us != 0) && ((now_us - s_last_us) < 5000U)) {
        s_prev_wireless_ready = wireless_ready;
        return;
    }
    s_last_us = now_us;
    s_prev_wireless_ready = wireless_ready;

    (void)svc_position_get(&pos);
    if ((keys_dev != 0) && (keys_dev->ops != 0) && (keys_dev->ops->read != 0)) {
        (void)keys_dev->ops->read(keys_dev->priv, keys, sizeof(keys), &n);
    }
    if ((adc_dev != 0) && (adc_dev->ops != 0) && (adc_dev->ops->read != 0)) {
        (void)adc_dev->ops->read(adc_dev->priv, adc, sizeof(adc), &n);
    }
    if ((temp_dev != 0) && (temp_dev->ops != 0) && (temp_dev->ops->read != 0)) {
        (void)temp_dev->ops->read(temp_dev->priv, temp, sizeof(temp), &n);
    }

    (void)svc_binding_get(0, &key_map0);
    mapped_keys = keys[0];

    pkt[0] = 0xE0U;
    pkt[1] = mapped_keys;
    pkt[2] = adc[0];
    pkt[3] = adc[1];
    pkt[4] = adc[2];
    pkt[5] = adc[3];
    pkt[6] = (uint8_t)(pos.x & 0xFFU);
    pkt[7] = (uint8_t)((pos.x >> 8) & 0xFFU);
    pkt[8] = (uint8_t)(pos.y & 0xFFU);
    pkt[9] = (uint8_t)((pos.y >> 8) & 0xFFU);
    pkt[10] = (uint8_t)((pos.run_mode << 1) | (pos.valid & 0x01U));
    pkt[11] = temp[0];
    pkt[12] = temp[1];
    pkt[13] = (uint8_t)svc_calibration_get_state();
    pkt[14] = key_map0;
    pkt[15] = (uint8_t)of_sm_get_mode();
    (void)of_link_send_telemetry(pkt, 16U, &sent);
    of_diag_on_tx(sent);
}

static void of_monitor_periph_health(void)
{
    static uint8_t degraded = 0;
    static uint8_t bad_cnt = 0;
    static uint8_t good_cnt = 0;
    int critical_ok = drv_ir_cam_is_ready() && drv_input_adc_is_ready() && drv_input_keys_is_ready();

    if (!critical_ok) {
        if (bad_cnt < 10U) {
            bad_cnt++;
        }
        good_cnt = 0;
        if ((bad_cnt >= 3U) && (degraded == 0U)) {
            degraded = 1U;
            of_sm_post_event(EVT_PERIPH_DEGRADED);
        }
        return;
    }

    if (good_cnt < 10U) {
        good_cnt++;
    }
    bad_cnt = 0;
    if ((good_cnt >= 5U) && (degraded != 0U)) {
        degraded = 0U;
        of_sm_post_event(EVT_PERIPH_RECOVERED);
    }
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
    static of_wireless_stream_t s_wireless_rx;
    static uint8_t s_wireless_inited = 0U;
    uint8_t pkt_type = 0U;
    uint8_t payload[OF_WPKT_MAX_PAYLOAD];
    uint32_t payload_len = 0U;
    static uint8_t s_logged_stream_error = 0U;

    if (s_wireless_inited == 0U) {
        of_wireless_stream_init(&s_wireless_rx);
        s_wireless_inited = 1U;
    }

    (void)svc_transport_route_tick();
    of_link_tick();

    if (of_transport_read(rx, sizeof(rx), &got) == 0 && got > 0) {
        of_diag_on_rx(got);
        if (of_link_wireless_active()) {
            of_wireless_stream_feed(&s_wireless_rx, rx, got);
            for (;;) {
                int stream_rc = of_wireless_stream_next(&s_wireless_rx, &pkt_type, payload, sizeof(payload),
                    &payload_len);

                if (stream_rc == 0) {
                    break;
                }
                if (stream_rc < 0) {
                    if (s_logged_stream_error == 0U) {
                        s_logged_stream_error = 1U;
                        osal_printk("[openfire][gun] malformed wireless frame dropped\r\n");
                    }
                    continue;
                }
                if (of_link_on_wireless_packet(pkt_type, payload, payload_len)) {
                    continue;
                }
                if (pkt_type == OF_WPKT_TYPE_SERIAL_TUNNEL) {
                    if (!of_try_handle_stress_ping(payload, payload_len)) {
                        of_proto_docked_process(payload, payload_len);
                        of_proto_mh_process(payload, payload_len);
                    }
                }
            }
        } else if (!of_try_handle_stress_ping(rx, got)) {
            of_proto_docked_process(rx, got);
            of_proto_mh_process(rx, got);
        }
    }

    if (OF_ENABLE_HEARTBEAT) {
        uint32_t sent = 0;
        t0 = (uapi_tcxo_get_us != 0) ? uapi_tcxo_get_us() : 0;
        if (OF_ROLE_DONGLE) {
            (void)of_link_send_serial(hb_dongle, sizeof(hb_dongle), &sent);
        } else {
            (void)of_link_send_serial(hb_gun, sizeof(hb_gun), &sent);
        }
        t1 = (uapi_tcxo_get_us != 0) ? uapi_tcxo_get_us() : 0;
        of_diag_on_tx(sent);
        if (t1 > t0) {
            of_diag_on_rtt_us((uint32_t)(t1 - t0));
        }
    }

    of_sm_step();
    (void)svc_position_poll();
    svc_usb_hid_tick();
    of_monitor_periph_health();
    of_publish_runtime_telemetry();
    of_diag_tick();
}
