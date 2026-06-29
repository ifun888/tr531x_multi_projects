#include "services/svc_position.h"
#include "services/svc_calibration.h"
#include "services/svc_profile.h"
#include "drivers/drv_ir_cam.h"
#include <string.h>

#define OF_POS_HIST_SZ 3U
#define OF_POS_SCREEN_MAX_X 1919U
#define OF_POS_SCREEN_MAX_Y 1079U
#define OF_POS_SPRING_DIRECT_DELTA 24
#define OF_POS_SPRING_MAX_STEP 96

static of_pos_sample_t g_sample;
static uint16_t g_hist_x[OF_POS_HIST_SZ];
static uint16_t g_hist_y[OF_POS_HIST_SZ];
static uint8_t g_hist_valid[OF_POS_HIST_SZ];
static uint8_t g_hist_count;
static uint8_t g_hist_head;

static uint16_t pos_clamp_u16(int32_t value, uint16_t max_value)
{
    if (value < 0) {
        return 0U;
    }
    if ((uint32_t)value > (uint32_t)max_value) {
        return max_value;
    }
    return (uint16_t)value;
}

static uint16_t pos_apply_center_shift(uint16_t raw, uint16_t center, uint16_t screen_max)
{
    uint16_t screen_center = (uint16_t)(screen_max / 2U);
    int32_t shifted = (int32_t)raw + (int32_t)screen_center - (int32_t)center;
    return pos_clamp_u16(shifted, screen_max);
}

static uint16_t pos_map_axis(uint16_t raw, uint16_t low, uint16_t high, uint16_t screen_max)
{
    uint32_t span;
    uint32_t num;

    if (high <= low) {
        return pos_clamp_u16(raw, screen_max);
    }
    if (raw <= low) {
        return 0U;
    }
    if (raw >= high) {
        return screen_max;
    }

    span = (uint32_t)high - (uint32_t)low;
    num = ((uint32_t)raw - (uint32_t)low) * (uint32_t)screen_max;
    return (uint16_t)(num / span);
}

static uint16_t pos_apply_spring_axis(uint16_t target, uint16_t last, uint16_t screen_max)
{
    int32_t delta = (int32_t)target - (int32_t)last;
    int32_t step;

    if ((delta <= OF_POS_SPRING_DIRECT_DELTA) && (delta >= -OF_POS_SPRING_DIRECT_DELTA)) {
        return target;
    }

    step = (delta * 70) / 100;
    if (step == 0) {
        step = (delta > 0) ? 1 : -1;
    }
    if (step > OF_POS_SPRING_MAX_STEP) {
        step = OF_POS_SPRING_MAX_STEP;
    } else if (step < -OF_POS_SPRING_MAX_STEP) {
        step = -OF_POS_SPRING_MAX_STEP;
    }
    return pos_clamp_u16((int32_t)last + step, screen_max);
}

static void pos_push_history(uint16_t x, uint16_t y)
{
    g_hist_x[g_hist_head] = x;
    g_hist_y[g_hist_head] = y;
    g_hist_valid[g_hist_head] = 1U;
    g_hist_head = (uint8_t)((g_hist_head + 1U) % OF_POS_HIST_SZ);
    if (g_hist_count < OF_POS_HIST_SZ) {
        g_hist_count++;
    }
}

static uint16_t pos_hist_x(uint8_t back)
{
    uint8_t idx = (uint8_t)((g_hist_head + OF_POS_HIST_SZ - 1U - back) % OF_POS_HIST_SZ);
    return g_hist_x[idx];
}

static uint16_t pos_hist_y(uint8_t back)
{
    uint8_t idx = (uint8_t)((g_hist_head + OF_POS_HIST_SZ - 1U - back) % OF_POS_HIST_SZ);
    return g_hist_y[idx];
}

static void pos_apply_spring_filter(uint16_t *x, uint16_t *y)
{
    if ((x == 0) || (y == 0) || (g_hist_count == 0U)) {
        return;
    }

    *x = pos_apply_spring_axis(*x, pos_hist_x(0U), OF_POS_SCREEN_MAX_X);
    *y = pos_apply_spring_axis(*y, pos_hist_y(0U), OF_POS_SCREEN_MAX_Y);
}

void svc_position_init(void)
{
    svc_position_reset();
}

void svc_position_reset(void)
{
    (void)memset(&g_sample, 0, sizeof(g_sample));
    (void)memset(g_hist_x, 0, sizeof(g_hist_x));
    (void)memset(g_hist_y, 0, sizeof(g_hist_y));
    (void)memset(g_hist_valid, 0, sizeof(g_hist_valid));
    g_hist_count = 0U;
    g_hist_head = 0U;
}

of_pos_run_mode_t svc_position_get_run_mode(void)
{
    uint8_t mode = svc_profile_get_run_mode();
    if (mode > (uint8_t)OF_POS_RUN_AVERAGE2) {
        mode = (uint8_t)OF_POS_RUN_NORMAL;
    }
    return (of_pos_run_mode_t)mode;
}

int svc_position_poll(void)
{
    uint8_t ir[8] = {0};
    uint32_t got = 0U;
    uint16_t cal_x = 0U;
    uint16_t cal_y = 0U;
    uint16_t top = 0U;
    uint16_t bottom = 0U;
    uint16_t left = 0U;
    uint16_t right = 0U;
    uint16_t mapped_x;
    uint16_t mapped_y;
    of_pos_run_mode_t mode;
    const of_dev_t *ir_dev = drv_ir_cam_get_dev();

    if ((ir_dev == 0) || (ir_dev->ops == 0) || (ir_dev->ops->read == 0)) {
        g_sample.valid = 0U;
        return -1;
    }
    if (ir_dev->ops->read(ir_dev->priv, ir, sizeof(ir), &got) != 0 || got < 5U) {
        g_sample.valid = 0U;
        return -1;
    }

    g_sample.raw_x = (uint16_t)ir[0] | ((uint16_t)ir[1] << 8);
    g_sample.raw_y = (uint16_t)ir[2] | ((uint16_t)ir[3] << 8);
    g_sample.valid = (ir[4] & 0x01U) ? 1U : 0U;
    (void)svc_calibration_get_result(&cal_x, &cal_y);
    (void)svc_calibration_get_offsets(&top, &bottom, &left, &right);
    mode = svc_position_get_run_mode();
    g_sample.run_mode = (uint8_t)mode;

    if (g_sample.valid == 0U) {
        return 0;
    }

    if ((right > left) && (bottom > top)) {
        mapped_x = pos_map_axis(g_sample.raw_x, left, right, OF_POS_SCREEN_MAX_X);
        mapped_y = pos_map_axis(g_sample.raw_y, top, bottom, OF_POS_SCREEN_MAX_Y);
    } else {
        mapped_x = pos_apply_center_shift(g_sample.raw_x, cal_x, OF_POS_SCREEN_MAX_X);
        mapped_y = pos_apply_center_shift(g_sample.raw_y, cal_y, OF_POS_SCREEN_MAX_Y);
    }

    pos_apply_spring_filter(&mapped_x, &mapped_y);

    pos_push_history(mapped_x, mapped_y);
    switch (mode) {
        case OF_POS_RUN_AVERAGE:
            if (g_hist_count >= 2U) {
                g_sample.x = (uint16_t)((pos_hist_x(0U) + pos_hist_x(1U)) / 2U);
                g_sample.y = (uint16_t)((pos_hist_y(0U) + pos_hist_y(1U)) / 2U);
            } else {
                g_sample.x = mapped_x;
                g_sample.y = mapped_y;
            }
            break;
        case OF_POS_RUN_AVERAGE2:
            if (g_hist_count >= 3U) {
                g_sample.x = (uint16_t)((mapped_x + pos_hist_x(0U) + pos_hist_x(1U) + pos_hist_x(2U)) / 4U);
                g_sample.y = (uint16_t)((mapped_y + pos_hist_y(0U) + pos_hist_y(1U) + pos_hist_y(2U)) / 4U);
            } else if (g_hist_count >= 2U) {
                g_sample.x = (uint16_t)((pos_hist_x(0U) + pos_hist_x(1U)) / 2U);
                g_sample.y = (uint16_t)((pos_hist_y(0U) + pos_hist_y(1U)) / 2U);
            } else {
                g_sample.x = mapped_x;
                g_sample.y = mapped_y;
            }
            break;
        case OF_POS_RUN_NORMAL:
        default:
            g_sample.x = mapped_x;
            g_sample.y = mapped_y;
            break;
    }

    return 0;
}

int svc_position_get(of_pos_sample_t *sample)
{
    if (sample == 0) {
        return -1;
    }
    *sample = g_sample;
    return 0;
}
