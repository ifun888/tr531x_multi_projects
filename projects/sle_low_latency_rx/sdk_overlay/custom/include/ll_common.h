#ifndef LL_COMMON_H
#define LL_COMMON_H

#include <stdint.h>
#include "platform_core.h"

#define LL_TEST_MAGIC0 0x4c
#define LL_TEST_MAGIC1 0x54
#define LL_TEST_VERSION 0x01
#define LL_MARK_PULSE_US 2000U
#define LL_RX_MARK_GPIO S_MGPIO12

void ll_mark_init(pin_t pin);
void ll_mark_pulse_start(pin_t pin);
void ll_mark_poll(pin_t pin);

void ll_rx_init(void);
void ll_rx_poll(void);
void ll_rx_process_packet(const uint8_t *data, uint16_t len);

#endif
