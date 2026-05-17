#ifndef DRV_IR_CAM_H
#define DRV_IR_CAM_H

#include "of_fops.h"

const of_dev_t *drv_ir_cam_get_dev(void);
int drv_ir_cam_is_ready(void);

#endif
