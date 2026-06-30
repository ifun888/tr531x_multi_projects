#ifndef DRV_IR_CAM_H
#define DRV_IR_CAM_H

#include "of_fops.h"
#include <stdint.h>

typedef struct {
    uint16_t screen_x;
    uint16_t screen_y;
    uint16_t raw_center_x;
    uint16_t raw_center_y;
    uint16_t point_spacing;
    uint8_t valid;
    uint8_t seen_count;
    uint8_t degraded;
} drv_ir_cam_solution_t;

const of_dev_t *drv_ir_cam_get_dev(void);
int drv_ir_cam_is_ready(void);
int drv_ir_cam_get_latest_solution(drv_ir_cam_solution_t *solution);

#endif
