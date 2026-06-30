#ifndef DRV_STORAGE_H
#define DRV_STORAGE_H

#include "of_fops.h"

const of_dev_t *drv_storage_get_dev(void);
int drv_storage_is_ready(void);

#endif
