#include "of_transport.h"
#include "of_protocol.h"
#include "of_diag.h"
#include "drivers/drv_usb_cdc.h"
#include "drivers/drv_usb_hid.h"
#include "of_link_io.h"
#include "of_wireless_pkt.h"
#include "osal_debug.h"
#include <stdint.h>
#include <string.h>

#ifndef OF_ROLE_DONGLE
#define OF_ROLE_DONGLE 0
#endif
#ifndef OF_ENABLE_HEARTBEAT
#define OF_ENABLE_HEARTBEAT 0
#endif

#define OF_MH_REPLAY_MAX_CMD_LEN   8U
#define OF_MH_REPLAY_PARSE_BUF_LEN 64U

typedef enum {
    OF_MH_SLOT_SESSION = 0,
    OF_MH_SLOT_MODE_EQ,
    OF_MH_SLOT_M0,
    OF_MH_SLOT_M1,
    OF_MH_SLOT_M2,
    OF_MH_SLOT_M3,
    OF_MH_SLOT_M6,
    OF_MH_SLOT_M8,
    OF_MH_SLOT_XI,
    OF_MH_SLOT_XR,
    OF_MH_SLOT_R0,
    OF_MH_SLOT_R1,
    OF_MH_SLOT_R2,
    OF_MH_SLOT_R3,
    OF_MH_SLOT_R4,
    OF_MH_SLOT_F0,
    OF_MH_SLOT_F1,
    OF_MH_SLOT_F2,
    OF_MH_SLOT_F3,
    OF_MH_SLOT_F4,
    OF_MH_SLOT_FD,
    OF_MH_SLOT_COUNT
} of_mh_replay_slot_t;

typedef struct {
    uint8_t valid;
    uint8_t len;
    uint8_t data[OF_MH_REPLAY_MAX_CMD_LEN];
} of_mh_replay_cmd_t;

typedef struct {
    uint8_t session_on;
    uint8_t pending_replay;
    uint8_t prev_link_ready;
    uint8_t parser_len;
    uint8_t parser[OF_MH_REPLAY_PARSE_BUF_LEN];
    of_mh_replay_cmd_t slots[OF_MH_SLOT_COUNT];
} of_mh_replay_ctx_t;

static of_mh_replay_ctx_t g_mh_replay;

static void of_mh_replay_drop_prefix(uint8_t count)
{
    if ((count == 0U) || (g_mh_replay.parser_len == 0U)) {
        return;
    }
    if (count >= g_mh_replay.parser_len) {
        g_mh_replay.parser_len = 0U;
        return;
    }
    (void)memmove(g_mh_replay.parser, &g_mh_replay.parser[count], g_mh_replay.parser_len - count);
    g_mh_replay.parser_len = (uint8_t)(g_mh_replay.parser_len - count);
}

static void of_mh_replay_clear_slot(of_mh_replay_slot_t slot)
{
    g_mh_replay.slots[slot].valid = 0U;
    g_mh_replay.slots[slot].len = 0U;
    (void)memset(g_mh_replay.slots[slot].data, 0, sizeof(g_mh_replay.slots[slot].data));
}

static void of_mh_replay_clear_all(void)
{
    uint32_t i;

    g_mh_replay.session_on = 0U;
    g_mh_replay.pending_replay = 0U;
    for (i = 0U; i < OF_MH_SLOT_COUNT; i++) {
        of_mh_replay_clear_slot((of_mh_replay_slot_t)i);
    }
}

static void of_mh_replay_store(of_mh_replay_slot_t slot, const uint8_t *buf, uint8_t len)
{
    if ((buf == 0) || (len == 0U) || (len > OF_MH_REPLAY_MAX_CMD_LEN)) {
        return;
    }

    g_mh_replay.slots[slot].valid = 1U;
    g_mh_replay.slots[slot].len = len;
    (void)memcpy(g_mh_replay.slots[slot].data, buf, len);
}

static int of_mh_replay_has_cached_state(void)
{
    uint32_t i;

    for (i = 0U; i < OF_MH_SLOT_COUNT; i++) {
        if (g_mh_replay.slots[i].valid != 0U) {
            return 1;
        }
    }
    return 0;
}

static void of_mh_replay_cache_command(const uint8_t *buf, uint8_t len)
{
    uint8_t code;

    if ((buf == 0) || (len == 0U)) {
        return;
    }

    if (buf[0] == 'E') {
        of_mh_replay_clear_all();
        return;
    }
    if (buf[0] == 'S') {
        g_mh_replay.session_on = 1U;
        of_mh_replay_store(OF_MH_SLOT_SESSION, buf, len);
        return;
    }
    if ((len >= 6U) && (memcmp(buf, "MODE=", 5U) == 0)) {
        of_mh_replay_store(OF_MH_SLOT_MODE_EQ, buf, len);
        return;
    }
    if ((buf[0] == 'M') && (len >= 4U)) {
        switch (buf[1]) {
            case '0':
                of_mh_replay_store(OF_MH_SLOT_M0, buf, len);
                return;
            case '1':
                of_mh_replay_store(OF_MH_SLOT_M1, buf, len);
                return;
            case '2':
                of_mh_replay_store(OF_MH_SLOT_M2, buf, len);
                return;
            case '3':
                of_mh_replay_store(OF_MH_SLOT_M3, buf, len);
                return;
            case '6':
                of_mh_replay_store(OF_MH_SLOT_M6, buf, len);
                return;
            case '8':
                of_mh_replay_store(OF_MH_SLOT_M8, buf, len);
                return;
            default:
                return;
        }
    }
    if ((buf[0] == 'X') && (len >= 3U)) {
        if (buf[1] == 'I') {
            of_mh_replay_store(OF_MH_SLOT_XI, buf, len);
        } else if (buf[1] == 'R') {
            of_mh_replay_store(OF_MH_SLOT_XR, buf, len);
        }
        return;
    }
    if ((buf[0] == 'R') && (len >= 4U)) {
        code = (uint8_t)(buf[1] - '0');
        if (code < 5U) {
            of_mh_replay_store((of_mh_replay_slot_t)(OF_MH_SLOT_R0 + code), buf, len);
        }
        return;
    }
    if ((buf[0] == 'F') && (len >= 4U)) {
        switch (buf[1]) {
            case '0':
                of_mh_replay_store(OF_MH_SLOT_F0, buf, len);
                return;
            case '1':
                of_mh_replay_store(OF_MH_SLOT_F1, buf, len);
                return;
            case '2':
                of_mh_replay_store(OF_MH_SLOT_F2, buf, len);
                return;
            case '3':
                of_mh_replay_store(OF_MH_SLOT_F3, buf, len);
                return;
            case '4':
                of_mh_replay_store(OF_MH_SLOT_F4, buf, len);
                return;
            case 'D':
                if (len >= 5U) {
                    of_mh_replay_store(OF_MH_SLOT_FD, buf, len);
                }
                return;
            default:
                return;
        }
    }
}

static void of_mh_replay_parse_buffer(void)
{
    uint8_t cmd_len;
    const uint8_t *buf = g_mh_replay.parser;

    while (g_mh_replay.parser_len != 0U) {
        cmd_len = 0U;
        buf = g_mh_replay.parser;

        if ((buf[0] == '\r') || (buf[0] == '\n') || (buf[0] == '\0')) {
            of_mh_replay_drop_prefix(1U);
            continue;
        }
        if ((g_mh_replay.parser_len >= 4U) && (memcmp(buf, "PING", 4U) == 0)) {
            of_mh_replay_drop_prefix(4U);
            continue;
        }
        if ((g_mh_replay.parser_len >= 5U) && (memcmp(buf, "MODE?", 5U) == 0)) {
            of_mh_replay_drop_prefix(5U);
            continue;
        }
        if ((g_mh_replay.parser_len >= 7U) && (memcmp(buf, "STATUS?", 7U) == 0)) {
            of_mh_replay_drop_prefix(7U);
            continue;
        }
        if ((g_mh_replay.parser_len >= 6U) && (memcmp(buf, "MODE=", 5U) == 0) &&
            (buf[5] >= '0') && (buf[5] <= '9')) {
            cmd_len = 6U;
        } else if ((buf[0] == 'S') || (buf[0] == 'E')) {
            cmd_len = 1U;
        } else if (buf[0] == 'M') {
            if (g_mh_replay.parser_len < 4U) {
                break;
            }
            if ((buf[2] == '=') && (buf[3] >= '0') && (buf[3] <= '9') &&
                ((buf[1] == '0') || (buf[1] == '1') || (buf[1] == '2') ||
                (buf[1] == '3') || (buf[1] == '6') || (buf[1] == '8'))) {
                cmd_len = 4U;
                if ((buf[1] == '0') && (buf[3] == '1')) {
                    if (g_mh_replay.parser_len >= 5U) {
                        if (buf[4] == 'L') {
                            cmd_len = 5U;
                        }
                    } else {
                        break;
                    }
                }
            }
        } else if (buf[0] == 'X') {
            if (g_mh_replay.parser_len < 3U) {
                break;
            }
            if (((buf[1] == 'I') || (buf[1] == 'R')) &&
                (buf[2] >= '0') && (buf[2] <= '9')) {
                cmd_len = 3U;
            }
        } else if (buf[0] == 'R') {
            if (g_mh_replay.parser_len < 4U) {
                break;
            }
            if ((buf[1] >= '0') && (buf[1] <= '4')) {
                cmd_len = 4U;
            }
        } else if (buf[0] == 'F') {
            if (g_mh_replay.parser_len < 4U) {
                break;
            }
            if ((buf[1] >= '0') && (buf[1] <= '4')) {
                cmd_len = 4U;
            } else if (buf[1] == 'D') {
                if (g_mh_replay.parser_len < 5U) {
                    break;
                }
                cmd_len = 5U;
            }
        }

        if (cmd_len != 0U) {
            of_mh_replay_cache_command(buf, cmd_len);
            of_mh_replay_drop_prefix(cmd_len);
            continue;
        }

        of_mh_replay_drop_prefix(1U);
    }
}

static void of_mh_replay_observe_usb_rx(const uint8_t *buf, uint32_t len)
{
    uint32_t copy_len;

    if ((buf == 0) || (len == 0U)) {
        return;
    }
    if ((g_mh_replay.parser_len + len) > OF_MH_REPLAY_PARSE_BUF_LEN) {
        g_mh_replay.parser_len = 0U;
    }

    copy_len = len;
    if (copy_len > (uint32_t)(OF_MH_REPLAY_PARSE_BUF_LEN - g_mh_replay.parser_len)) {
        copy_len = (uint32_t)(OF_MH_REPLAY_PARSE_BUF_LEN - g_mh_replay.parser_len);
    }
    if (copy_len == 0U) {
        return;
    }

    (void)memcpy(&g_mh_replay.parser[g_mh_replay.parser_len], buf, copy_len);
    g_mh_replay.parser_len = (uint8_t)(g_mh_replay.parser_len + copy_len);
    of_mh_replay_parse_buffer();
}

static int of_mh_replay_flush_cached_state(void)
{
    static const of_mh_replay_slot_t replay_order[] = {
        OF_MH_SLOT_SESSION,
        OF_MH_SLOT_MODE_EQ,
        OF_MH_SLOT_M0,
        OF_MH_SLOT_M1,
        OF_MH_SLOT_M2,
        OF_MH_SLOT_M3,
        OF_MH_SLOT_M6,
        OF_MH_SLOT_M8,
        OF_MH_SLOT_XI,
        OF_MH_SLOT_XR,
        OF_MH_SLOT_R0,
        OF_MH_SLOT_R1,
        OF_MH_SLOT_R2,
        OF_MH_SLOT_R3,
        OF_MH_SLOT_R4,
        OF_MH_SLOT_F0,
        OF_MH_SLOT_F1,
        OF_MH_SLOT_F2,
        OF_MH_SLOT_F3,
        OF_MH_SLOT_F4,
        OF_MH_SLOT_FD,
    };
    uint32_t i;
    uint32_t sent;
    const of_mh_replay_cmd_t *cmd;

    if (!of_link_is_ready()) {
        return -1;
    }

    for (i = 0U; i < (sizeof(replay_order) / sizeof(replay_order[0])); i++) {
        cmd = &g_mh_replay.slots[replay_order[i]];
        if (cmd->valid == 0U) {
            continue;
        }
        sent = 0U;
        if (of_link_send_serial(cmd->data, cmd->len, &sent) != 0) {
            return -1;
        }
    }

    return 0;
}

static void of_mh_replay_tick(void)
{
    uint8_t link_ready = (uint8_t)(of_link_is_ready() ? 1U : 0U);

    if ((g_mh_replay.prev_link_ready == 0U) && (link_ready != 0U) && of_mh_replay_has_cached_state()) {
        g_mh_replay.pending_replay = 1U;
        osal_printk("[openfire][dongle] sle link ready, schedule mh replay\r\n");
    }
    g_mh_replay.prev_link_ready = link_ready;

    if ((g_mh_replay.pending_replay != 0U) && (link_ready != 0U)) {
        if (of_mh_replay_flush_cached_state() == 0) {
            g_mh_replay.pending_replay = 0U;
            osal_printk("[openfire][dongle] mh state replayed after reconnect\r\n");
        }
    }
}

static void of_release_hid_on_disconnect(void)
{
    static uint8_t s_prev_link_ready = 0U;
    static uint8_t s_pending_release = 0U;
    uint8_t link_ready = (uint8_t)(of_link_is_ready() ? 1U : 0U);

    if ((s_prev_link_ready != 0U) && (link_ready == 0U)) {
        s_pending_release = 1U;
        osal_printk("[openfire][dongle] sle link lost, schedule hid release\r\n");
    }
    s_prev_link_ready = link_ready;

    if ((s_pending_release != 0U) && drv_usb_hid_is_ready()) {
        if (drv_usb_hid_release_all() == 0) {
            s_pending_release = 0U;
            osal_printk("[openfire][dongle] hid release sent after disconnect\r\n");
        }
    }
}

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
    of_mh_replay_observe_usb_rx(rx, got);
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
    of_mh_replay_tick();
    of_release_hid_on_disconnect();
    of_bridge_usb_to_sle();
    of_bridge_sle_to_usb_and_hid();
    of_diag_tick();
}
