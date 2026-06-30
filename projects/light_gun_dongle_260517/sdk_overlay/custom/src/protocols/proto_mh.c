#include "of_protocol.h"
#include "of_transport.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    uint8_t session_on;
    uint8_t ffb_solenoid;
    uint8_t ffb_rumble;
    uint8_t rgb_r;
    uint8_t rgb_g;
    uint8_t rgb_b;
    uint8_t burst[5];
    char display_tag;
    uint8_t display_value;
    uint8_t input_mode;
    uint8_t offscreen_mode;
    uint8_t pedal_mode;
    uint8_t ar_correction;
    uint8_t rumble_mode;
    uint8_t autofire_mode;
    uint8_t autofire_interval;
    uint8_t player_slot;
} of_mh_state_t;

static of_mh_state_t g_mh;

static void mh_reply(const char *s)
{
    uint32_t sent = 0;
    (void)of_transport_write((const uint8_t *)s, (uint32_t)strlen(s), &sent);
}

static uint8_t mh_hex_nibble(uint8_t c)
{
    if (c >= '0' && c <= '9') {
        return (uint8_t)(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return (uint8_t)(c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F') {
        return (uint8_t)(c - 'A' + 10);
    }
    return 0;
}

static uint8_t mh_hex_byte(const uint8_t *buf, uint32_t len, uint32_t pos)
{
    if ((buf == 0) || (pos + 1U >= len)) {
        return 0;
    }
    return (uint8_t)((mh_hex_nibble(buf[pos]) << 4) | mh_hex_nibble(buf[pos + 1U]));
}

static void mh_reply_status(void)
{
    char rsp[96];
    int n = snprintf(rsp, sizeof(rsp),
        "MH S=%u M=%u F0=%u F1=%u RGB=%u,%u,%u D=%c:%u I=%u O=%u P=%u A=%u R=%u AF=%u AI=%u PL=%u\n",
        (unsigned)g_mh.session_on, (unsigned)of_proto_get_mode(),
        (unsigned)g_mh.ffb_solenoid, (unsigned)g_mh.ffb_rumble,
        (unsigned)g_mh.rgb_r, (unsigned)g_mh.rgb_g, (unsigned)g_mh.rgb_b,
        g_mh.display_tag ? g_mh.display_tag : '-', (unsigned)g_mh.display_value,
        (unsigned)g_mh.input_mode, (unsigned)g_mh.offscreen_mode,
        (unsigned)g_mh.pedal_mode, (unsigned)g_mh.ar_correction,
        (unsigned)g_mh.rumble_mode, (unsigned)g_mh.autofire_mode,
        (unsigned)g_mh.autofire_interval, (unsigned)g_mh.player_slot);
    if (n > 0) {
        uint32_t sent = 0;
        (void)of_transport_write((const uint8_t *)rsp, (uint32_t)n, &sent);
    }
}

void of_proto_mh_process(const uint8_t *buf, uint32_t len)
{
    if ((buf == 0) || (len == 0U)) {
        return;
    }

    if ((len >= 4U) && (memcmp(buf, "PING", 4) == 0)) {
        mh_reply("PONG\n");
        return;
    }
    if ((len >= 5U) && (memcmp(buf, "MODE?", 5) == 0)) {
        char rsp[20];
        int n = snprintf(rsp, sizeof(rsp), "MODE=%u\n", (unsigned)of_proto_get_mode());
        if (n > 0) {
            uint32_t sent = 0;
            (void)of_transport_write((const uint8_t *)rsp, (uint32_t)n, &sent);
        }
        return;
    }
    if ((len >= 6U) && (memcmp(buf, "MODE=", 5) == 0)) {
        of_proto_set_mode((of_mode_t)(buf[5] - '0'));
        mh_reply("OK\n");
        return;
    }
    if ((len >= 7U) && (memcmp(buf, "STATUS?", 7) == 0)) {
        mh_reply_status();
        return;
    }
    if ((len >= 2U) && (buf[0] == 'M')) {
        if ((buf[1] == '0') && (len >= 4U)) {
            g_mh.input_mode = (uint8_t)(buf[3] - '0');
            mh_reply("MODE\n");
            return;
        }
        if ((buf[1] == '1') && (len >= 4U)) {
            g_mh.offscreen_mode = (uint8_t)(buf[3] - '0');
            mh_reply("MODE\n");
            return;
        }
        if ((buf[1] == '2') && (len >= 4U)) {
            g_mh.pedal_mode = (uint8_t)(buf[3] - '0');
            mh_reply("MODE\n");
            return;
        }
        if ((buf[1] == '3') && (len >= 4U)) {
            g_mh.ar_correction = (uint8_t)(buf[3] - '0');
            mh_reply("MODE\n");
            return;
        }
        if ((buf[1] == '6') && (len >= 4U)) {
            g_mh.rumble_mode = (uint8_t)(buf[3] - '0');
            mh_reply("MODE\n");
            return;
        }
        if ((buf[1] == '8') && (len >= 4U)) {
            g_mh.autofire_mode = (uint8_t)(buf[3] - '0');
            mh_reply("MODE\n");
            return;
        }
        switch (buf[1]) {
            case '0':
            case '8':
                of_proto_set_mode(OF_MODE_RUN);
                break;
            case '1':
                of_proto_set_mode(OF_MODE_PAUSE);
                break;
            case '2':
            case '6':
                of_proto_set_mode(OF_MODE_CALIB);
                break;
            case 'D':
                of_proto_set_mode(OF_MODE_DOCKED);
                break;
            default:
                break;
        }
        mh_reply("MODE\n");
        return;
    }
    if ((len >= 2U) && (buf[0] == 'F')) {
        switch (buf[1]) {
            case '0':
                g_mh.ffb_solenoid = (len >= 4U) ? mh_hex_byte(buf, len, 2U) : 0U;
                break;
            case '1':
                g_mh.ffb_rumble = (len >= 4U) ? mh_hex_byte(buf, len, 2U) : 0U;
                break;
            case '2':
                g_mh.rgb_r = (len >= 4U) ? mh_hex_byte(buf, len, 2U) : 0U;
                break;
            case '3':
                g_mh.rgb_g = (len >= 4U) ? mh_hex_byte(buf, len, 2U) : 0U;
                break;
            case '4':
                g_mh.rgb_b = (len >= 4U) ? mh_hex_byte(buf, len, 2U) : 0U;
                break;
            case 'D':
                g_mh.display_tag = (len >= 3U) ? (char)buf[2] : '-';
                g_mh.display_value = (len >= 5U) ? mh_hex_byte(buf, len, 3U) : 0U;
                break;
            default:
                break;
        }
        mh_reply("FFB\n");
        return;
    }
    if ((len >= 2U) && (buf[0] == 'R')) {
        uint8_t idx = (uint8_t)(buf[1] - '0');
        if (idx < 5U) {
            g_mh.burst[idx] = (len >= 4U) ? mh_hex_byte(buf, len, 2U) : 0U;
        }
        mh_reply("RPT\n");
        return;
    }
    if ((len >= 2U) && (buf[0] == 'X')) {
        if ((buf[1] == 'I') && (len >= 3U)) {
            g_mh.autofire_interval = (uint8_t)(buf[2] - '0');
            mh_reply("CFG\n");
            return;
        }
        if ((buf[1] == 'R') && (len >= 3U)) {
            g_mh.player_slot = (uint8_t)(buf[2] - '0');
            mh_reply("CFG\n");
            return;
        }
    }
    if (buf[0] == 'S') {
        g_mh.session_on = 1U;
        mh_reply("START\n");
    } else if (buf[0] == 'E') {
        g_mh.session_on = 0U;
        g_mh.ar_correction = 0U;
        g_mh.offscreen_mode = 0U;
        g_mh.pedal_mode = 0U;
        mh_reply("END\n");
    }
}
