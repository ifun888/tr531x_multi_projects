#ifndef DRV_TEMP_SENSOR_H
#define DRV_TEMP_SENSOR_H

#include "of_fops.h"

const of_dev_t *drv_temp_sensor_get_dev(void);
int drv_temp_sensor_is_ready(void);

#endif
