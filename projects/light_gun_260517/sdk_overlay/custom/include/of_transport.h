#ifndef OF_TRANSPORT_H
#define OF_TRANSPORT_H

#include <stdint.h>

typedef enum {
    OF_TRANSPORT_USB_CDC = 0,
    OF_TRANSPORT_SLE,
} of_transport_type_t;

int of_transport_init(of_transport_type_t type);
int of_transport_read(uint8_t *buf, uint32_t len, uint32_t *out_len);
int of_transport_write(const uint8_t *buf, uint32_t len, uint32_t *out_len);
int of_transport_deinit(void);
of_transport_type_t of_transport_get_type(void);

#endif
