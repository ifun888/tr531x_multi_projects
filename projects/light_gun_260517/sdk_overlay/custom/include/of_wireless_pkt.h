#ifndef OF_WIRELESS_PKT_H
#define OF_WIRELESS_PKT_H

#include <stdint.h>

#define OF_WPKT_SOF0 0xA5U
#define OF_WPKT_SOF1 0x5AU
#define OF_WPKT_MAX_PAYLOAD 240U
#define OF_WPKT_STREAM_BUF_SZ 512U

typedef enum {
    OF_WPKT_TYPE_LINK_HELLO = 0x10,
    OF_WPKT_TYPE_LINK_HELLO_ACK = 0x11,
    OF_WPKT_TYPE_SERIAL_TUNNEL = 1,
    OF_WPKT_TYPE_HID_MOUSE = 2,
    OF_WPKT_TYPE_HID_KEYBOARD = 3,
    OF_WPKT_TYPE_HID_GAMEPAD = 4,
    OF_WPKT_TYPE_TELEMETRY = 5,
} of_wireless_pkt_type_t;

typedef struct {
    uint8_t buttons;
    int8_t dx;
    int8_t dy;
    int8_t wheel;
} of_wpkt_mouse_payload_t;

typedef struct {
    uint8_t key_count;
    uint8_t keys[6];
} of_wpkt_keyboard_payload_t;

typedef struct __attribute__((packed)) {
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t rz;
    int16_t rx;
    int16_t ry;
    uint8_t hat;
    uint32_t buttons;
} of_wpkt_gamepad_payload_t;

typedef struct {
    uint8_t buf[OF_WPKT_STREAM_BUF_SZ];
    uint32_t len;
} of_wireless_stream_t;

int of_wireless_pkt_encode(uint8_t type, const uint8_t *payload, uint32_t payload_len,
    uint8_t *out, uint32_t out_cap, uint32_t *out_len);
void of_wireless_stream_init(of_wireless_stream_t *stream);
void of_wireless_stream_feed(of_wireless_stream_t *stream, const uint8_t *data, uint32_t len);
int of_wireless_stream_next(of_wireless_stream_t *stream, uint8_t *type,
    uint8_t *payload, uint32_t payload_cap, uint32_t *payload_len);

#endif
