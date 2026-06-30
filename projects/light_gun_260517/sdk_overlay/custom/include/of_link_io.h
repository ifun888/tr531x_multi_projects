#ifndef OF_LINK_IO_H
#define OF_LINK_IO_H

#include <stdint.h>

int of_link_wireless_active(void);
int of_link_is_ready(void);
void of_link_tick(void);
int of_link_on_wireless_packet(uint8_t type, const uint8_t *payload, uint32_t payload_len);
int of_link_send_packet(uint8_t type, const uint8_t *payload, uint32_t payload_len, uint32_t *out_len);
int of_link_send_serial(const uint8_t *buf, uint32_t len, uint32_t *out_len);
int of_link_send_telemetry(const uint8_t *buf, uint32_t len, uint32_t *out_len);

#endif
