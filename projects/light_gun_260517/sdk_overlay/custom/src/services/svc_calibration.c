#include "services/svc_calibration.h"
#include "drivers/drv_storage.h"

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
    const of_dev_t *st = drv_storage_get_dev();
    uint8_t raw[8];
    uint32_t out = 0;
    if (g_state != OF_CAL_DONE) {
        return -1;
    }
    if ((st == 0) || (st->ops == 0) || (st->ops->write == 0)) {
        return -1;
    }
    raw[0] = 0x43;
    raw[1] = 0x41;
    raw[2] = 0x4C;
    raw[3] = 0x31;
    raw[4] = (uint8_t)(g_cx & 0xFFU);
    raw[5] = (uint8_t)((g_cx >> 8) & 0xFFU);
    raw[6] = (uint8_t)(g_cy & 0xFFU);
    raw[7] = (uint8_t)((g_cy >> 8) & 0xFFU);
    if (st->ops->write(st->priv, raw, sizeof(raw), &out) != 0) {
        return -1;
    }
    return (out == sizeof(raw)) ? 0 : -1;
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
