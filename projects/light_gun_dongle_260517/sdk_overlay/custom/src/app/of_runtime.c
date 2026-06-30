#include "of_transport.h"
#include "of_protocol.h"
#include "of_diag.h"
#include "drivers/drv_usb_cdc.h"
#include "drivers/drv_usb_hid.h"
#include "of_link_io.h"
#include "of_wireless_pkt.h"
#include "osal_debug.h"
#include <stdint.h>

#ifndef OF_ROLE_DONGLE
#define OF_ROLE_DONGLE 0
#endif
#ifndef OF_ENABLE_HEARTBEAT
#define OF_ENABLE_HEARTBEAT 0
#endif

static void of_bridge_usb_to_sle(void)
{
    const of_dev_t *usb = drv_usb_cdc_get_dev();
    uint8_t rx[64];
    uint32_t got = 0U;
    uint32_t sent = 0U;

    if ((usb == 0) || (usb->ops == 0) || (usb->ops->read == 0)) {
        return;
    }
    if (!of_link_is_ready()) {
        return;
    }
    if (usb->ops->read(usb->priv, rx, sizeof(rx), &got) != 0 || got == 0U) {
        return;
    }
    of_diag_on_rx(got);
    (void)of_link_send_packet(OF_WPKT_TYPE_SERIAL_TUNNEL, rx, got, &sent);
    of_diag_on_tx(sent);
}

static void of_bridge_sle_to_usb_and_hid(void)
{
    const of_dev_t *usb = drv_usb_cdc_get_dev();
    uint8_t rx[64];
    uint32_t got = 0U;
    static of_wireless_stream_t s_wireless_rx;
    static uint8_t s_wireless_inited = 0U;
    uint8_t pkt_type = 0U;
    uint8_t payload[OF_WPKT_MAX_PAYLOAD];
    uint32_t payload_len = 0U;
    static uint8_t s_logged_unsupported[256] = {0};
    static uint8_t s_logged_stream_error = 0U;

    if (s_wireless_inited == 0U) {
        of_wireless_stream_init(&s_wireless_rx);
        s_wireless_inited = 1U;
    }

    if (of_transport_read(rx, sizeof(rx), &got) != 0 || got == 0U) {
        return;
    }
    of_diag_on_rx(got);
    of_wireless_stream_feed(&s_wireless_rx, rx, got);

    for (;;) {
        int stream_rc = of_wireless_stream_next(&s_wireless_rx, &pkt_type, payload, sizeof(payload), &payload_len);

        if (stream_rc == 0) {
            break;
        }
        if (stream_rc < 0) {
            if (s_logged_stream_error == 0U) {
                s_logged_stream_error = 1U;
                osal_printk("[openfire][dongle] malformed wireless frame dropped\r\n");
            }
            continue;
        }
        if (of_link_on_wireless_packet(pkt_type, payload, payload_len)) {
            continue;
        }
        if ((pkt_type == OF_WPKT_TYPE_SERIAL_TUNNEL) &&
            (usb != 0) && (usb->ops != 0) && (usb->ops->write != 0)) {
            uint32_t sent = 0U;
            (void)usb->ops->write(usb->priv, payload, payload_len, &sent);
            of_diag_on_tx(sent);
        } else if ((pkt_type == OF_WPKT_TYPE_TELEMETRY) &&
            (usb != 0) && (usb->ops != 0) && (usb->ops->write != 0)) {
            uint32_t sent = 0U;
            (void)usb->ops->write(usb->priv, payload, payload_len, &sent);
            of_diag_on_tx(sent);
        } else if ((pkt_type == OF_WPKT_TYPE_HID_MOUSE) && (payload_len == sizeof(of_wpkt_mouse_payload_t))) {
            const of_wpkt_mouse_payload_t *pkt = (const of_wpkt_mouse_payload_t *)payload;
            (void)drv_usb_hid_send_mouse_report(pkt->buttons, pkt->dx, pkt->dy, pkt->wheel);
        } else if ((pkt_type == OF_WPKT_TYPE_HID_KEYBOARD) && (payload_len == sizeof(of_wpkt_keyboard_payload_t))) {
            const of_wpkt_keyboard_payload_t *pkt = (const of_wpkt_keyboard_payload_t *)payload;
            (void)drv_usb_hid_send_keyboard_report(pkt->keys, pkt->key_count);
        } else if ((pkt_type == OF_WPKT_TYPE_HID_GAMEPAD) && (payload_len == sizeof(of_wpkt_gamepad_payload_t))) {
            (void)drv_usb_hid_send_gamepad_report(payload, payload_len);
        } else if (s_logged_unsupported[pkt_type] == 0U) {
            s_logged_unsupported[pkt_type] = 1U;
            osal_printk("[openfire][dongle] unsupported wireless pkt type=%u len=%u\r\n",
                (unsigned int)pkt_type, (unsigned int)payload_len);
        }
    }
}

void of_runtime_once(void)
{
    of_link_tick();
    of_bridge_usb_to_sle();
    of_bridge_sle_to_usb_and_hid();
    of_diag_tick();
}
