#include "services/svc_calibration.h"

#include "services/svc_profile.h"

#define OF_CAL_STAGE_SAMPLES 16U

typedef struct {
    uint32_t sum_x;
    uint32_t sum_y;
    uint32_t count;
} of_cal_accum_t;

static of_cal_state_t g_state = OF_CAL_IDLE;
static of_cal_accum_t g_accum;

static uint16_t g_center_x;
static uint16_t g_center_y;
static uint16_t g_top_offset;
static uint16_t g_bottom_offset;
static uint16_t g_left_offset;
static uint16_t g_right_offset;

static uint16_t g_pending_center_x;
static uint16_t g_pending_center_y;
static uint16_t g_pending_top_offset;
static uint16_t g_pending_bottom_offset;
static uint16_t g_pending_left_offset;
static uint16_t g_pending_right_offset;

static void cal_reset_accum(void)
{
    g_accum.sum_x = 0U;
    g_accum.sum_y = 0U;
    g_accum.count = 0U;
}

static void cal_reset_pending(void)
{
    g_pending_center_x = 0U;
    g_pending_center_y = 0U;
    g_pending_top_offset = 0U;
    g_pending_bottom_offset = 0U;
    g_pending_left_offset = 0U;
    g_pending_right_offset = 0U;
    cal_reset_accum();
}

static uint16_t cal_avg_x(void)
{
    return (g_accum.count != 0U) ? (uint16_t)(g_accum.sum_x / g_accum.count) : 0U;
}

static uint16_t cal_avg_y(void)
{
    return (g_accum.count != 0U) ? (uint16_t)(g_accum.sum_y / g_accum.count) : 0U;
}

static void cal_finish_stage(void)
{
    switch (g_state) {
        case OF_CAL_TOP:
            g_pending_top_offset = cal_avg_y();
            g_state = OF_CAL_BOTTOM;
            break;
        case OF_CAL_BOTTOM:
            g_pending_bottom_offset = cal_avg_y();
            g_state = OF_CAL_LEFT;
            break;
        case OF_CAL_LEFT:
            g_pending_left_offset = cal_avg_x();
            g_state = OF_CAL_RIGHT;
            break;
        case OF_CAL_RIGHT:
            g_pending_right_offset = cal_avg_x();
            g_state = OF_CAL_CENTER;
            break;
        case OF_CAL_CENTER:
            g_pending_center_x = cal_avg_x();
            g_pending_center_y = cal_avg_y();
            g_state = OF_CAL_VERIFY;
            break;
        default:
            break;
    }
    cal_reset_accum();
}

int svc_calibration_enter(void)
{
    cal_reset_pending();
    g_state = OF_CAL_TOP;
    return 0;
}

int svc_calibration_exit(void)
{
    cal_reset_pending();
    g_state = OF_CAL_IDLE;
    return 0;
}

int svc_calibration_push_sample(uint16_t x, uint16_t y)
{
    if ((g_state < OF_CAL_TOP) || (g_state > OF_CAL_CENTER)) {
        return -1;
    }

    g_accum.sum_x += x;
    g_accum.sum_y += y;
    g_accum.count++;
    if (g_accum.count >= OF_CAL_STAGE_SAMPLES) {
        cal_finish_stage();
    }
    return 0;
}

int svc_calibration_commit(void)
{
    if (g_state != OF_CAL_VERIFY) {
        return -1;
    }

    g_center_x = g_pending_center_x;
    g_center_y = g_pending_center_y;
    g_top_offset = g_pending_top_offset;
    g_bottom_offset = g_pending_bottom_offset;
    g_left_offset = g_pending_left_offset;
    g_right_offset = g_pending_right_offset;

    svc_profile_set_ir_center(g_center_x, g_center_y);
    svc_profile_set_ir_offsets(g_top_offset, g_bottom_offset, g_left_offset, g_right_offset);
    g_state = OF_CAL_DONE;
    return svc_profile_save();
}

int svc_calibration_get_state(void)
{
    return (int)g_state;
}

int svc_calibration_get_result(uint16_t *cx, uint16_t *cy)
{
    if ((cx == 0) || (cy == 0)) {
        return -1;
    }

    if (g_state == OF_CAL_VERIFY) {
        *cx = g_pending_center_x;
        *cy = g_pending_center_y;
    } else {
        *cx = g_center_x;
        *cy = g_center_y;
    }
    return 0;
}

int svc_calibration_get_offsets(uint16_t *top, uint16_t *bottom, uint16_t *left, uint16_t *right)
{
    if ((top == 0) || (bottom == 0) || (left == 0) || (right == 0)) {
        return -1;
    }

    if (g_state == OF_CAL_VERIFY) {
        *top = g_pending_top_offset;
        *bottom = g_pending_bottom_offset;
        *left = g_pending_left_offset;
        *right = g_pending_right_offset;
    } else {
        *top = g_top_offset;
        *bottom = g_bottom_offset;
        *left = g_left_offset;
        *right = g_right_offset;
    }
    return 0;
}

int svc_calibration_load_profile(void)
{
    if (svc_profile_get_ir_center(&g_center_x, &g_center_y) != 0) {
        return -1;
    }
    if (svc_profile_get_ir_offsets(&g_top_offset, &g_bottom_offset, &g_left_offset, &g_right_offset) != 0) {
        return -1;
    }

    cal_reset_pending();
    g_state = ((g_center_x != 0U) || (g_center_y != 0U) ||
               (g_top_offset != 0U) || (g_bottom_offset != 0U) ||
               (g_left_offset != 0U) || (g_right_offset != 0U)) ? OF_CAL_DONE : OF_CAL_IDLE;
    return 0;
}
