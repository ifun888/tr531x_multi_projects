#ifndef DRV_RUMBLE_H
#define DRV_RUMBLE_H

#include <stdint.h>

int drv_rumble_init(void);
void drv_rumble_deinit(void);
int drv_rumble_is_ready(void);
int drv_rumble_set_level(uint8_t level);
int drv_rumble_pulse(uint8_t level, uint32_t active_ms);
void drv_rumble_stop(void);

#endif
