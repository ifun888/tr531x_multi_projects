#ifndef SVC_CALIBRATION_H
#define SVC_CALIBRATION_H

#include <stdint.h>

typedef enum {
    OF_CAL_IDLE = 0,
    OF_CAL_TOP,
    OF_CAL_BOTTOM,
    OF_CAL_LEFT,
    OF_CAL_RIGHT,
    OF_CAL_CENTER,
    OF_CAL_VERIFY,
    OF_CAL_DONE,
} of_cal_state_t;

int svc_calibration_enter(void);
int svc_calibration_exit(void);
int svc_calibration_push_sample(uint16_t x, uint16_t y);
int svc_calibration_commit(void);
int svc_calibration_get_state(void);
int svc_calibration_get_result(uint16_t *cx, uint16_t *cy);
int svc_calibration_get_offsets(uint16_t *top, uint16_t *bottom, uint16_t *left, uint16_t *right);
int svc_calibration_load_profile(void);

#endif
