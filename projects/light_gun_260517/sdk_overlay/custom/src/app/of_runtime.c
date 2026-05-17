#include "of_transport.h"
#include "of_protocol.h"
#include "of_diag.h"
#include "drivers/drv_ir_cam.h"
#include "drivers/drv_input_keys.h"
#include "drivers/drv_input_adc.h"
#include "drivers/drv_temp_sensor.h"
#include "drivers/drv_feedback.h"
#include "drivers/drv_led.h"
#include "of_sm.h"
#include "services/svc_binding.h"
#include "services/svc_calibration.h"
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

static void of_publish_runtime_telemetry(void)
{
    extern uint64_t uapi_tcxo_get_us(void) __attribute__((weak));
    static uint64_t s_last_us = 0;
    uint64_t now_us = (uapi_tcxo_get_us != 0) ? uapi_tcxo_get_us() : 0;
    uint8_t pkt[20] = {0};
    uint8_t ir[8] = {0};
    uint8_t keys[2] = {0};
    uint8_t adc[4] = {0};
    uint8_t temp[2] = {0};
    uint32_t n = 0;
    uint32_t sent = 0;
    uint16_t cal_x = 0;
    uint16_t cal_y = 0;
    uint16_t ir_x = 0;
    uint16_t ir_y = 0;
    uint8_t key_map0 = 0;
    uint8_t mapped_keys = 0;
    const of_dev_t *ir_dev = drv_ir_cam_get_dev();
    const of_dev_t *keys_dev = drv_input_keys_get_dev();
    const of_dev_t *adc_dev = drv_input_adc_get_dev();
    const of_dev_t *temp_dev = drv_temp_sensor_get_dev();

    if ((now_us != 0) && (s_last_us != 0) && ((now_us - s_last_us) < 5000U)) {
        return;
    }
    s_last_us = now_us;

    if ((ir_dev != 0) && (ir_dev->ops != 0) && (ir_dev->ops->read != 0)) {
        (void)ir_dev->ops->read(ir_dev->priv, ir, sizeof(ir), &n);
    }
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
    (void)svc_calibration_get_result(&cal_x, &cal_y);
    ir_x = (uint16_t)ir[0] | ((uint16_t)ir[1] << 8);
    ir_y = (uint16_t)ir[2] | ((uint16_t)ir[3] << 8);
    if (ir_x > cal_x) {
        ir_x = (uint16_t)(ir_x - cal_x);
    } else {
        ir_x = 0;
    }
    if (ir_y > cal_y) {
        ir_y = (uint16_t)(ir_y - cal_y);
    } else {
        ir_y = 0;
    }
    mapped_keys = (keys[0] != 0U) ? key_map0 : 0U;

    pkt[0] = 0xE0U;
    pkt[1] = mapped_keys;
    pkt[2] = adc[0];
    pkt[3] = adc[1];
    pkt[4] = adc[2];
    pkt[5] = adc[3];
    pkt[6] = (uint8_t)(ir_x & 0xFFU);
    pkt[7] = (uint8_t)((ir_x >> 8) & 0xFFU);
    pkt[8] = (uint8_t)(ir_y & 0xFFU);
    pkt[9] = (uint8_t)((ir_y >> 8) & 0xFFU);
    pkt[10] = ir[4];
    pkt[11] = temp[0];
    pkt[12] = temp[1];
    pkt[13] = (uint8_t)svc_calibration_get_state();
    pkt[14] = key_map0;
    pkt[15] = (uint8_t)of_sm_get_mode();
    (void)of_transport_write(pkt, 16U, &sent);
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

    of_sm_step();
    of_monitor_periph_health();
    of_publish_runtime_telemetry();
    of_diag_tick();
}
