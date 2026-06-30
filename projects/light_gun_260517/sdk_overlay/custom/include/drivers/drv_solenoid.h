#ifndef DRV_SOLENOID_H
#define DRV_SOLENOID_H

#include <stdint.h>

int drv_solenoid_init(void);
void drv_solenoid_deinit(void);
int drv_solenoid_is_ready(void);
int drv_solenoid_fire_once(uint32_t on_ms);
int drv_solenoid_fire_burst(uint8_t shots, uint32_t on_ms, uint32_t off_ms);
int drv_solenoid_fire_level(uint8_t level);
void drv_solenoid_stop(void);

#endif
