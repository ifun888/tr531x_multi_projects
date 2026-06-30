#include "of_wireless_pkt.h"

#include <string.h>

static uint8_t of_wireless_pkt_crc(uint8_t type, uint8_t len, const uint8_t *payload)
{
    uint8_t crc = (uint8_t)(type ^ len);
    uint32_t i;

    for (i = 0; i < len; i++) {
        crc ^= payload[i];
    }
    return crc;
}

int of_wireless_pkt_encode(uint8_t type, const uint8_t *payload, uint32_t payload_len,
    uint8_t *out, uint32_t out_cap, uint32_t *out_len)
{
    uint8_t len8;

    if (out_len != 0) {
        *out_len = 0U;
    }
    if ((out == 0) || (out_len == 0) || (payload_len > OF_WPKT_MAX_PAYLOAD)) {
        return -1;
    }

    len8 = (uint8_t)payload_len;
    if (out_cap < (uint32_t)(5U + len8)) {
        return -1;
    }

    out[0] = OF_WPKT_SOF0;
    out[1] = OF_WPKT_SOF1;
    out[2] = type;
    out[3] = len8;
    if ((payload != 0) && (len8 > 0U)) {
        (void)memcpy(&out[4], payload, len8);
    }
    out[4U + len8] = of_wireless_pkt_crc(type, len8, payload != 0 ? payload : out);
    *out_len = 5U + len8;
    return 0;
}

void of_wireless_stream_init(of_wireless_stream_t *stream)
{
    if (stream == 0) {
        return;
    }
    (void)memset(stream, 0, sizeof(*stream));
}

void of_wireless_stream_feed(of_wireless_stream_t *stream, const uint8_t *data, uint32_t len)
{
    uint32_t keep;

    if ((stream == 0) || (data == 0) || (len == 0U)) {
        return;
    }

    if (len >= OF_WPKT_STREAM_BUF_SZ) {
        data += (len - OF_WPKT_STREAM_BUF_SZ);
        len = OF_WPKT_STREAM_BUF_SZ;
        stream->len = 0U;
    } else if ((stream->len + len) > OF_WPKT_STREAM_BUF_SZ) {
        keep = OF_WPKT_STREAM_BUF_SZ - len;
        if ((keep > 0U) && (stream->len > keep)) {
            (void)memmove(stream->buf, &stream->buf[stream->len - keep], keep);
        }
        stream->len = keep;
    }

    (void)memcpy(&stream->buf[stream->len], data, len);
    stream->len += len;
}

int of_wireless_stream_next(of_wireless_stream_t *stream, uint8_t *type,
    uint8_t *payload, uint32_t payload_cap, uint32_t *payload_len)
{
    uint32_t i;
    uint32_t frame_len;
    uint8_t len8;
    uint8_t crc;

    if (payload_len != 0) {
        *payload_len = 0U;
    }
    if ((stream == 0) || (type == 0) || (payload_len == 0)) {
        return 0;
    }

    i = 0U;
    while ((i + 1U) < stream->len) {
        if ((stream->buf[i] == OF_WPKT_SOF0) && (stream->buf[i + 1U] == OF_WPKT_SOF1)) {
            break;
        }
        i++;
    }

    if (i > 0U) {
        (void)memmove(stream->buf, &stream->buf[i], stream->len - i);
        stream->len -= i;
    }

    if (stream->len < 5U) {
        return 0;
    }

    len8 = stream->buf[3];
    frame_len = 5U + len8;
    if (stream->len < frame_len) {
        return 0;
    }
    if (len8 > payload_cap) {
        (void)memmove(stream->buf, &stream->buf[frame_len], stream->len - frame_len);
        stream->len -= frame_len;
        return -1;
    }

    crc = of_wireless_pkt_crc(stream->buf[2], len8, &stream->buf[4]);
    if (crc != stream->buf[4U + len8]) {
        (void)memmove(stream->buf, &stream->buf[1], stream->len - 1U);
        stream->len -= 1U;
        return -1;
    }

    *type = stream->buf[2];
    if ((payload != 0) && (len8 > 0U)) {
        (void)memcpy(payload, &stream->buf[4], len8);
    }
    *payload_len = len8;

    (void)memmove(stream->buf, &stream->buf[frame_len], stream->len - frame_len);
    stream->len -= frame_len;
    return 1;
}
