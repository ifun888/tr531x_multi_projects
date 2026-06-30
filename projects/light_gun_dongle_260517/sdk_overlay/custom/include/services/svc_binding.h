#ifndef SVC_BINDING_H
#define SVC_BINDING_H

#include <stdint.h>

int svc_binding_apply_default(void);
int svc_binding_load(void);
int svc_binding_save(void);
int svc_binding_set(uint8_t slot, uint8_t code);
int svc_binding_get(uint8_t slot, uint8_t *code);
const uint8_t *svc_binding_data(uint32_t *len);

#endif
