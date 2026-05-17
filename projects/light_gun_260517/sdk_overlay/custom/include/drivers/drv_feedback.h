#ifndef DRV_FEEDBACK_H
#define DRV_FEEDBACK_H

#include "of_fops.h"

const of_dev_t *drv_feedback_get_dev(void);
int drv_feedback_is_ready(void);

#endif
