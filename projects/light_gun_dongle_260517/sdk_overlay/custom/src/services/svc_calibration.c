#include "services/svc_calibration.h"
#include "drivers/drv_storage.h"
#include "services/svc_profile.h"

static of_cal_state_t g_state = OF_CAL_IDLE;
static uint32_t g_cnt = 0;
static uint32_t g_sum_x = 0;
static uint32_t g_sum_y = 0;
static uint16_t g_cx = 0;
static uint16_t g_cy = 0;

int svc_calibration_enter(void)
{
    g_state = OF_CAL_RUNNING;
    g_cnt = 0;
    g_sum_x = 0;
    g_sum_y = 0;
    return 0;
}

int svc_calibration_exit(void)
{
    g_state = OF_CAL_IDLE;
    g_cnt = 0;
    g_sum_x = 0;
    g_sum_y = 0;
    g_cx = 0;
    g_cy = 0;
    return 0;
}

int svc_calibration_push_sample(uint16_t x, uint16_t y)
{
    if (g_state != OF_CAL_RUNNING) {
        return -1;
    }
    g_sum_x += x;
    g_sum_y += y;
    g_cnt++;
    if (g_cnt >= 16U) {
        g_cx = (uint16_t)(g_sum_x / g_cnt);
        g_cy = (uint16_t)(g_sum_y / g_cnt);
        g_state = OF_CAL_DONE;
    }
    return 0;
}

int svc_calibration_commit(void)
{
    if (g_state != OF_CAL_DONE) {
        return -1;
    }
    svc_profile_set_calibration(g_cx, g_cy);
    return svc_profile_save();
}

int svc_calibration_get_state(void)
{
    return (int)g_state;
}

int svc_calibration_get_result(uint16_t *cx, uint16_t *cy)
{
    if (cx == 0 || cy == 0) {
        return -1;
    }
    *cx = g_cx;
    *cy = g_cy;
    return 0;
}

int svc_calibration_load_profile(void)
{
    uint16_t cx = 0;
    uint16_t cy = 0;

    if (svc_profile_get_calibration(&cx, &cy) != 0) {
        return -1;
    }
    g_cx = cx;
    g_cy = cy;
    g_cnt = 0;
    g_sum_x = 0;
    g_sum_y = 0;
    g_state = ((cx != 0U) || (cy != 0U)) ? OF_CAL_DONE : OF_CAL_IDLE;
    return 0;
}
