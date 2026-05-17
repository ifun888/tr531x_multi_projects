#ifndef DRV_INPUT_KEYS_H
#define DRV_INPUT_KEYS_H

#include "of_fops.h"

const of_dev_t *drv_input_keys_get_dev(void);
int drv_input_keys_is_ready(void);

#endif
