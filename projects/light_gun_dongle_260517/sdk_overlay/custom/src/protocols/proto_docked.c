#include "of_protocol.h"
#include "of_transport.h"

static of_mode_t g_mode = OF_MODE_RUN;
static uint8_t g_cfg_blob[16] = {0};

void of_proto_set_mode(of_mode_t mode)
{
    g_mode = mode;
}

of_mode_t of_proto_get_mode(void)
{
    return g_mode;
}

static void proto_reply(const uint8_t *p, uint32_t n)
{
    uint32_t sent = 0;
    (void)of_transport_write(p, n, &sent);
}

void of_proto_docked_process(const uint8_t *buf, uint32_t len)
{
    if ((buf == 0) || (len == 0U)) {
        return;
    }

    if ((len >= 2U) && (buf[0] == 0x01U) && (buf[1] == 0x02U)) {
        static const uint8_t ack[] = {0x81, 0x02, 0x00};
        proto_reply(ack, sizeof(ack));
        return;
    }

    if ((len >= 2U) && (buf[0] == 0x10U)) {
        g_mode = (of_mode_t)buf[1];
        {
            uint8_t ack[] = {0x90, (uint8_t)g_mode, 0x00};
            proto_reply(ack, sizeof(ack));
        }
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x11U)) {
        uint8_t rsp[] = {0x91, (uint8_t)g_mode};
        proto_reply(rsp, sizeof(rsp));
        return;
    }

    if ((len >= 3U) && (buf[0] == 0x20U)) {
        if (buf[1] < sizeof(g_cfg_blob)) {
            g_cfg_blob[buf[1]] = buf[2];
            {
                uint8_t ack[] = {0xA0, buf[1], g_cfg_blob[buf[1]], 0x00};
                proto_reply(ack, sizeof(ack));
            }
        }
        return;
    }

    if ((len >= 2U) && (buf[0] == 0x21U)) {
        if (buf[1] < sizeof(g_cfg_blob)) {
            uint8_t rsp[] = {0xA1, buf[1], g_cfg_blob[buf[1]]};
            proto_reply(rsp, sizeof(rsp));
        }
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x30U)) {
        uint8_t rsp[] = {0xB0, (uint8_t)g_mode, 0x01};
        proto_reply(rsp, sizeof(rsp));
    }
}
