#include "of_protocol.h"
#include "of_transport.h"
#include <string.h>
#include <stdio.h>

static void mh_reply(const char *s)
{
    uint32_t sent = 0;
    (void)of_transport_write((const uint8_t *)s, (uint32_t)strlen(s), &sent);
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
    if (buf[0] == 'S') {
        mh_reply("START\n");
    } else if (buf[0] == 'E') {
        mh_reply("END\n");
    }
}
