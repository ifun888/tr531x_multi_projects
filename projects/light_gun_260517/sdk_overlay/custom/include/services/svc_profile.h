#ifndef SVC_PROFILE_H
#define SVC_PROFILE_H

#include <stdint.h>

typedef enum {
    OF_CFG_TOGGLES = 0,
    OF_CFG_PINS,
    OF_CFG_SETTINGS,
    OF_CFG_PROFILE,
    OF_CFG_BUTTONS,
    OF_CFG_USB_ID,
} of_cfg_type_t;

int svc_profile_load(void);
int svc_profile_save(void);
void svc_profile_apply_default(void);
const uint8_t *svc_profile_get_blob(of_cfg_type_t type, uint32_t *len);
int svc_profile_set_blob(of_cfg_type_t type, const uint8_t *buf, uint32_t len);
int svc_profile_get_u32(of_cfg_type_t type, uint8_t index, uint32_t *value);
int svc_profile_set_u32(of_cfg_type_t type, uint8_t index, uint32_t value);
uint8_t svc_profile_get_run_mode(void);
void svc_profile_set_run_mode(uint8_t mode);
int svc_profile_get_calibration(uint16_t *cx, uint16_t *cy);
void svc_profile_set_calibration(uint16_t cx, uint16_t cy);
int svc_profile_get_ir_center(uint16_t *cx, uint16_t *cy);
void svc_profile_set_ir_center(uint16_t cx, uint16_t cy);
int svc_profile_get_ir_offsets(uint16_t *top, uint16_t *bottom, uint16_t *left, uint16_t *right);
void svc_profile_set_ir_offsets(uint16_t top, uint16_t bottom, uint16_t left, uint16_t right);

#endif
