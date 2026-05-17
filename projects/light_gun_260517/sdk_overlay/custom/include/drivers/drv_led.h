#ifndef DRV_LED_H
#define DRV_LED_H

#include "of_fops.h"

const of_dev_t *drv_led_get_dev(void);
int drv_led_is_ready(void);

#endif
