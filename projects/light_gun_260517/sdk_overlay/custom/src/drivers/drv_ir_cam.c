#include "drivers/drv_ir_cam.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "common_def.h"
#include "i2c.h"
#include "osal_debug.h"
#include "pinctrl.h"
#include "soc_osal.h"

#ifndef OF_IR_I2C_BUS
#ifdef CONFIG_LIGHT_GUN_260517_IR_I2C_BUS
#define OF_IR_I2C_BUS CONFIG_LIGHT_GUN_260517_IR_I2C_BUS
#else
#define OF_IR_I2C_BUS 0
#endif
#endif

#ifndef OF_IR_I2C_ADDR
#ifdef CONFIG_LIGHT_GUN_260517_IR_I2C_ADDR
#define OF_IR_I2C_ADDR CONFIG_LIGHT_GUN_260517_IR_I2C_ADDR
#else
#define OF_IR_I2C_ADDR 0x58
#endif
#endif

#ifndef OF_IR_I2C_BAUDRATE
#ifdef CONFIG_LIGHT_GUN_260517_IR_I2C_BAUDRATE
#define OF_IR_I2C_BAUDRATE CONFIG_LIGHT_GUN_260517_IR_I2C_BAUDRATE
#else
#define OF_IR_I2C_BAUDRATE 200000U
#endif
#endif

#ifndef OF_IR_SCL_PIN
#ifdef CONFIG_LIGHT_GUN_260517_IR_SCL_PIN
#define OF_IR_SCL_PIN CONFIG_LIGHT_GUN_260517_IR_SCL_PIN
#else
#define OF_IR_SCL_PIN 25
#endif
#endif

#ifndef OF_IR_SDA_PIN
#ifdef CONFIG_LIGHT_GUN_260517_IR_SDA_PIN
#define OF_IR_SDA_PIN CONFIG_LIGHT_GUN_260517_IR_SDA_PIN
#else
#define OF_IR_SDA_PIN 26
#endif
#endif

#ifndef OF_IR_SCL_PIN_MODE
#ifdef CONFIG_LIGHT_GUN_260517_IR_SCL_PIN_MODE
#define OF_IR_SCL_PIN_MODE CONFIG_LIGHT_GUN_260517_IR_SCL_PIN_MODE
#else
#define OF_IR_SCL_PIN_MODE 26
#endif
#endif

#ifndef OF_IR_SDA_PIN_MODE
#ifdef CONFIG_LIGHT_GUN_260517_IR_SDA_PIN_MODE
#define OF_IR_SDA_PIN_MODE CONFIG_LIGHT_GUN_260517_IR_SDA_PIN_MODE
#else
#define OF_IR_SDA_PIN_MODE 27
#endif
#endif

#ifndef OF_IR_SENSITIVITY
#ifdef CONFIG_LIGHT_GUN_260517_IR_SENSITIVITY
#define OF_IR_SENSITIVITY CONFIG_LIGHT_GUN_260517_IR_SENSITIVITY
#else
#define OF_IR_SENSITIVITY 0U
#endif
#endif

#define IR_BASIC_FRAME_LEN 11U
#define IR_POINT_MAX 4U
#define IR_CAMERA_MAX_X 1023U
#define IR_CAMERA_MAX_Y 767U
#define IR_SCREEN_MAX_X 1919U
#define IR_SCREEN_MAX_Y 1079U
#define IR_ATOMIC_RETRY 2U
#define IR_SMOOTH_PERCENT 35U
#define IR_MIN_POINTS_FOR_SOLVE 2U

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t valid;
} ir_point_t;

typedef struct {
    ir_point_t points[IR_POINT_MAX];
    uint8_t seen_mask;
} ir_raw_frame_t;

static int g_opened;
static int g_ready;
static uint16_t g_smooth_x;
static uint16_t g_smooth_y;
static uint8_t g_first_solution_ready;
static uint8_t g_ordered_corners_ready;
static uint16_t g_last_corner_x[IR_POINT_MAX];
static uint16_t g_last_corner_y[IR_POINT_MAX];
static drv_ir_cam_solution_t g_last_solution;

static uint8_t ir_popcount4(uint8_t value)
{
    uint8_t count = 0U;
    while (value != 0U) {
        count = (uint8_t)(count + (value & 0x01U));
        value >>= 1;
    }
    return count;
}

static int ir_i2c_write_bytes(const uint8_t *buf, uint32_t len)
{
    i2c_data_t data;
    errcode_t ret;

    (void)memset(&data, 0, sizeof(data));
    data.send_buf = (uint8_t *)buf;
    data.send_len = len;
    ret = uapi_i2c_master_write((i2c_bus_t)OF_IR_I2C_BUS, (uint16_t)OF_IR_I2C_ADDR, &data);
    return (ret == ERRCODE_SUCC) ? 0 : -1;
}

static int ir_i2c_read_reg(uint8_t reg, uint8_t *rx, uint32_t rx_len)
{
    i2c_data_t data;
    errcode_t ret;

    (void)memset(&data, 0, sizeof(data));
    data.send_buf = &reg;
    data.send_len = 1U;
    data.receive_buf = rx;
    data.receive_len = rx_len;
    ret = uapi_i2c_master_writeread((i2c_bus_t)OF_IR_I2C_BUS, (uint16_t)OF_IR_I2C_ADDR, &data);
    return (ret == ERRCODE_SUCC) ? 0 : -1;
}

static int ir_write_reg_pair(uint8_t reg, uint8_t value)
{
    uint8_t tx[2];
    tx[0] = reg;
    tx[1] = value;
    return ir_i2c_write_bytes(tx, sizeof(tx));
}

static void ir_fill_sensitivity(uint8_t sensitivity, uint8_t *reg06, uint8_t *reg08, uint8_t *reg1a)
{
    static const uint8_t reg06_table[3] = {0x90U, 0x90U, 0xFFU};
    static const uint8_t reg08_table[3] = {0xC0U, 0x41U, 0x0CU};
    static const uint8_t reg1a_table[3] = {0x40U, 0x40U, 0x00U};
    uint8_t index = sensitivity;

    if (index > 2U) {
        index = 2U;
    }
    *reg06 = reg06_table[index];
    *reg08 = reg08_table[index];
    *reg1a = reg1a_table[index];
}

static uint16_t ir_map_linear(uint16_t input, uint16_t in_max, uint16_t out_max)
{
    if (input >= in_max) {
        return out_max;
    }
    return (uint16_t)(((uint32_t)input * (uint32_t)out_max) / (uint32_t)in_max);
}

static uint32_t ir_dist_sq_u16(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    int32_t dx = (int32_t)x0 - (int32_t)x1;
    int32_t dy = (int32_t)y0 - (int32_t)y1;
    return (uint32_t)(dx * dx + dy * dy);
}

static uint16_t ir_clamp_coord_i32(int32_t value, int32_t max_value)
{
    if (value < 0) {
        return 0U;
    }
    if (value > max_value) {
        return (uint16_t)max_value;
    }
    return (uint16_t)value;
}

static uint32_t ir_abs_diff_u16(uint16_t a, uint16_t b)
{
    return (a >= b) ? (uint32_t)(a - b) : (uint32_t)(b - a);
}

static void ir_apply_smoothing(drv_ir_cam_solution_t *solution)
{
    uint32_t keep_percent = IR_SMOOTH_PERCENT;
    uint32_t new_percent = 100U - keep_percent;

    if (solution->valid == 0U) {
        return;
    }

    if ((g_first_solution_ready == 0U) || (keep_percent == 0U)) {
        g_smooth_x = solution->screen_x;
        g_smooth_y = solution->screen_y;
        g_first_solution_ready = 1U;
    } else {
        g_smooth_x = (uint16_t)(((uint32_t)g_smooth_x * keep_percent +
            (uint32_t)solution->screen_x * new_percent) / 100U);
        g_smooth_y = (uint16_t)(((uint32_t)g_smooth_y * keep_percent +
            (uint32_t)solution->screen_y * new_percent) / 100U);
    }

    solution->screen_x = g_smooth_x;
    solution->screen_y = g_smooth_y;
}

static int ir_validate_ordered_corners(const uint16_t *corner_x, const uint16_t *corner_y)
{
    uint32_t width_top_sq;
    uint32_t width_bottom_sq;
    uint32_t height_left_sq;
    uint32_t height_right_sq;
    uint32_t min_width_sq;
    uint32_t max_width_sq;
    uint32_t min_height_sq;
    uint32_t max_height_sq;
    int32_t area2;
    uint32_t top_avg_y = ((uint32_t)corner_y[0] + (uint32_t)corner_y[1]) / 2U;
    uint32_t bottom_avg_y = ((uint32_t)corner_y[2] + (uint32_t)corner_y[3]) / 2U;
    uint32_t left_avg_x = ((uint32_t)corner_x[0] + (uint32_t)corner_x[2]) / 2U;
    uint32_t right_avg_x = ((uint32_t)corner_x[1] + (uint32_t)corner_x[3]) / 2U;

    if ((top_avg_y + 8U) >= bottom_avg_y) {
        return 0;
    }
    if ((left_avg_x + 8U) >= right_avg_x) {
        return 0;
    }

    width_top_sq = ir_dist_sq_u16(corner_x[0], corner_y[0], corner_x[1], corner_y[1]);
    width_bottom_sq = ir_dist_sq_u16(corner_x[2], corner_y[2], corner_x[3], corner_y[3]);
    height_left_sq = ir_dist_sq_u16(corner_x[0], corner_y[0], corner_x[2], corner_y[2]);
    height_right_sq = ir_dist_sq_u16(corner_x[1], corner_y[1], corner_x[3], corner_y[3]);
    min_width_sq = (width_top_sq < width_bottom_sq) ? width_top_sq : width_bottom_sq;
    max_width_sq = (width_top_sq > width_bottom_sq) ? width_top_sq : width_bottom_sq;
    min_height_sq = (height_left_sq < height_right_sq) ? height_left_sq : height_right_sq;
    max_height_sq = (height_left_sq > height_right_sq) ? height_left_sq : height_right_sq;
    if ((min_width_sq < 400U) || (min_height_sq < 100U)) {
        return 0;
    }
    if (max_width_sq > (min_width_sq * 6U)) {
        return 0;
    }
    if (max_height_sq > (min_height_sq * 6U)) {
        return 0;
    }

    area2 =
        (int32_t)corner_x[0] * (int32_t)corner_y[1] - (int32_t)corner_y[0] * (int32_t)corner_x[1] +
        (int32_t)corner_x[1] * (int32_t)corner_y[3] - (int32_t)corner_y[1] * (int32_t)corner_x[3] +
        (int32_t)corner_x[3] * (int32_t)corner_y[2] - (int32_t)corner_y[3] * (int32_t)corner_x[2] +
        (int32_t)corner_x[2] * (int32_t)corner_y[0] - (int32_t)corner_y[2] * (int32_t)corner_x[0];
    if (area2 < 0) {
        area2 = -area2;
    }
    if (area2 < 2000) {
        return 0;
    }
    if ((ir_abs_diff_u16(corner_x[0], corner_x[2]) > 700U) &&
        (ir_abs_diff_u16(corner_x[1], corner_x[3]) > 700U)) {
        return 0;
    }
    return 1;
}

static void ir_order_quad_points(const uint16_t *src_x, const uint16_t *src_y, uint16_t *dst_x, uint16_t *dst_y)
{
    uint8_t used[IR_POINT_MAX] = {0U};
    uint8_t i;
    uint8_t best_idx;
    int32_t best_metric;
    int32_t metric;
    uint16_t tx;
    uint16_t ty;

    best_idx = 0U;
    best_metric = (int32_t)src_x[0] + (int32_t)src_y[0];
    for (i = 1U; i < IR_POINT_MAX; i++) {
        metric = (int32_t)src_x[i] + (int32_t)src_y[i];
        if (metric < best_metric) {
            best_metric = metric;
            best_idx = i;
        }
    }
    dst_x[0] = src_x[best_idx];
    dst_y[0] = src_y[best_idx];
    used[best_idx] = 1U;

    best_idx = 0U;
    best_metric = -1;
    for (i = 0U; i < IR_POINT_MAX; i++) {
        if (used[i] != 0U) {
            continue;
        }
        metric = (int32_t)src_x[i] + (int32_t)src_y[i];
        if (metric > best_metric) {
            best_metric = metric;
            best_idx = i;
        }
    }
    dst_x[3] = src_x[best_idx];
    dst_y[3] = src_y[best_idx];
    used[best_idx] = 1U;

    best_idx = 0U;
    best_metric = INT32_MIN;
    for (i = 0U; i < IR_POINT_MAX; i++) {
        if (used[i] != 0U) {
            continue;
        }
        metric = (int32_t)src_x[i] - (int32_t)src_y[i];
        if (metric > best_metric) {
            best_metric = metric;
            best_idx = i;
        }
    }
    dst_x[1] = src_x[best_idx];
    dst_y[1] = src_y[best_idx];
    used[best_idx] = 1U;

    for (i = 0U; i < IR_POINT_MAX; i++) {
        if (used[i] == 0U) {
            dst_x[2] = src_x[i];
            dst_y[2] = src_y[i];
            break;
        }
    }

    if (dst_y[0] > dst_y[2]) {
        tx = dst_x[0]; ty = dst_y[0];
        dst_x[0] = dst_x[2]; dst_y[0] = dst_y[2];
        dst_x[2] = tx; dst_y[2] = ty;
    }
    if (dst_y[1] > dst_y[3]) {
        tx = dst_x[1]; ty = dst_y[1];
        dst_x[1] = dst_x[3]; dst_y[1] = dst_y[3];
        dst_x[3] = tx; dst_y[3] = ty;
    }
    if (dst_x[0] > dst_x[1]) {
        tx = dst_x[0]; ty = dst_y[0];
        dst_x[0] = dst_x[1]; dst_y[0] = dst_y[1];
        dst_x[1] = tx; dst_y[1] = ty;
    }
    if (dst_x[2] > dst_x[3]) {
        tx = dst_x[2]; ty = dst_y[2];
        dst_x[2] = dst_x[3]; dst_y[2] = dst_y[3];
        dst_x[3] = tx; dst_y[3] = ty;
    }
}

static void ir_save_ordered_corners(const uint16_t *corner_x, const uint16_t *corner_y)
{
    uint8_t i;
    for (i = 0U; i < IR_POINT_MAX; i++) {
        g_last_corner_x[i] = corner_x[i];
        g_last_corner_y[i] = corner_y[i];
    }
    g_ordered_corners_ready = 1U;
}

static int ir_try_track_pair_from_previous(const ir_raw_frame_t *frame, uint16_t *corner_x, uint16_t *corner_y)
{
    uint8_t vis_idx[2];
    uint8_t vis_count = 0U;
    uint8_t i;
    uint8_t a;
    uint8_t b;
    uint8_t best_a = 0U;
    uint8_t best_b = 1U;
    uint8_t best_swap = 0U;
    uint32_t best_cost = UINT32_MAX;
    int32_t dx0;
    int32_t dy0;
    int32_t dx1;
    int32_t dy1;
    int32_t avg_dx;
    int32_t avg_dy;

    if (g_ordered_corners_ready == 0U) {
        return 0;
    }
    for (i = 0U; i < IR_POINT_MAX; i++) {
        if (frame->points[i].valid == 0U) {
            continue;
        }
        if (vis_count < 2U) {
            vis_idx[vis_count] = i;
        }
        vis_count++;
    }
    if (vis_count != 2U) {
        return 0;
    }

    for (a = 0U; a < IR_POINT_MAX; a++) {
        for (b = 0U; b < IR_POINT_MAX; b++) {
            uint32_t cost_direct;
            uint32_t cost_swap;
            if (a == b) {
                continue;
            }
            cost_direct =
                ir_dist_sq_u16(frame->points[vis_idx[0]].x, frame->points[vis_idx[0]].y,
                    g_last_corner_x[a], g_last_corner_y[a]) +
                ir_dist_sq_u16(frame->points[vis_idx[1]].x, frame->points[vis_idx[1]].y,
                    g_last_corner_x[b], g_last_corner_y[b]);
            if (cost_direct < best_cost) {
                best_cost = cost_direct;
                best_a = a;
                best_b = b;
                best_swap = 0U;
            }
            cost_swap =
                ir_dist_sq_u16(frame->points[vis_idx[0]].x, frame->points[vis_idx[0]].y,
                    g_last_corner_x[b], g_last_corner_y[b]) +
                ir_dist_sq_u16(frame->points[vis_idx[1]].x, frame->points[vis_idx[1]].y,
                    g_last_corner_x[a], g_last_corner_y[a]);
            if (cost_swap < best_cost) {
                best_cost = cost_swap;
                best_a = a;
                best_b = b;
                best_swap = 1U;
            }
        }
    }
    if (best_swap != 0U) {
        uint8_t tmp = best_a;
        best_a = best_b;
        best_b = tmp;
    }

    dx0 = (int32_t)frame->points[vis_idx[0]].x - (int32_t)g_last_corner_x[best_a];
    dy0 = (int32_t)frame->points[vis_idx[0]].y - (int32_t)g_last_corner_y[best_a];
    dx1 = (int32_t)frame->points[vis_idx[1]].x - (int32_t)g_last_corner_x[best_b];
    dy1 = (int32_t)frame->points[vis_idx[1]].y - (int32_t)g_last_corner_y[best_b];
    avg_dx = (dx0 + dx1) / 2;
    avg_dy = (dy0 + dy1) / 2;

    for (i = 0U; i < IR_POINT_MAX; i++) {
        corner_x[i] = ir_clamp_coord_i32((int32_t)g_last_corner_x[i] + avg_dx, (int32_t)IR_CAMERA_MAX_X);
        corner_y[i] = ir_clamp_coord_i32((int32_t)g_last_corner_y[i] + avg_dy, (int32_t)IR_CAMERA_MAX_Y);
    }
    corner_x[best_a] = frame->points[vis_idx[0]].x;
    corner_y[best_a] = frame->points[vis_idx[0]].y;
    corner_x[best_b] = frame->points[vis_idx[1]].x;
    corner_y[best_b] = frame->points[vis_idx[1]].y;
    return 1;
}

static int ir_reconstruct_three_points(const ir_raw_frame_t *frame, uint16_t *corner_x, uint16_t *corner_y)
{
    uint16_t px[4];
    uint16_t py[4];
    uint8_t i;
    uint8_t count = 0U;
    uint8_t a_idx;
    uint8_t b_idx;
    uint8_t c_idx;
    int32_t dx;
    int32_t dy;
    uint32_t d01_sq;
    uint32_t d12_sq;
    uint32_t d02_sq;

    for (i = 0U; i < IR_POINT_MAX; i++) {
        if (frame->points[i].valid == 0U) {
            continue;
        }
        px[count] = frame->points[i].x;
        py[count] = frame->points[i].y;
        count++;
    }
    if (count != 3U) {
        return 0;
    }

    dx = (int32_t)px[0] - (int32_t)px[1];
    dy = (int32_t)py[0] - (int32_t)py[1];
    d01_sq = (uint32_t)(dx * dx + dy * dy);
    dx = (int32_t)px[1] - (int32_t)px[2];
    dy = (int32_t)py[1] - (int32_t)py[2];
    d12_sq = (uint32_t)(dx * dx + dy * dy);
    dx = (int32_t)px[0] - (int32_t)px[2];
    dy = (int32_t)py[0] - (int32_t)py[2];
    d02_sq = (uint32_t)(dx * dx + dy * dy);

    if ((d01_sq >= d12_sq) && (d01_sq >= d02_sq)) {
        a_idx = 0U; c_idx = 1U; b_idx = 2U;
    } else if (d12_sq >= d02_sq) {
        a_idx = 1U; c_idx = 2U; b_idx = 0U;
    } else {
        a_idx = 0U; c_idx = 2U; b_idx = 1U;
    }

    px[3] = ir_clamp_coord_i32((int32_t)px[a_idx] + (int32_t)px[c_idx] - (int32_t)px[b_idx], (int32_t)IR_CAMERA_MAX_X);
    py[3] = ir_clamp_coord_i32((int32_t)py[a_idx] + (int32_t)py[c_idx] - (int32_t)py[b_idx], (int32_t)IR_CAMERA_MAX_Y);
    ir_order_quad_points(px, py, corner_x, corner_y);
    return 1;
}

static int ir_build_ordered_corners(const ir_raw_frame_t *frame, drv_ir_cam_solution_t *solution,
    uint16_t *corner_x, uint16_t *corner_y)
{
    uint16_t px[4];
    uint16_t py[4];
    uint8_t i;
    uint8_t count = 0U;

    if (solution->seen_count == 4U) {
        for (i = 0U; i < IR_POINT_MAX; i++) {
            if (frame->points[i].valid == 0U) {
                continue;
            }
            px[count] = frame->points[i].x;
            py[count] = frame->points[i].y;
            count++;
        }
        if (count != 4U) {
            return 0;
        }
        ir_order_quad_points(px, py, corner_x, corner_y);
        return ir_validate_ordered_corners(corner_x, corner_y);
    }

    if (solution->seen_count == 3U) {
        if (!ir_reconstruct_three_points(frame, corner_x, corner_y)) {
            return 0;
        }
        return ir_validate_ordered_corners(corner_x, corner_y);
    }

    if (solution->seen_count == 2U) {
        if (!ir_try_track_pair_from_previous(frame, corner_x, corner_y)) {
            return 0;
        }
        return ir_validate_ordered_corners(corner_x, corner_y);
    }

    return 0;
}

static void ir_unpack_basic_frame(const uint8_t *buf, ir_raw_frame_t *frame)
{
    uint8_t high;
    uint16_t y;
    uint32_t pair_index;

    (void)memset(frame, 0, sizeof(*frame));
    for (pair_index = 0U; pair_index < 2U; pair_index++) {
        uint32_t base = 1U + pair_index * 5U;
        uint32_t point_a = pair_index * 2U;
        uint32_t point_b = point_a + 1U;

        high = buf[base + 2U];

        y = (uint16_t)buf[base + 1U] | (uint16_t)((high & 0xC0U) << 2U);
        if (y <= IR_CAMERA_MAX_Y) {
            frame->points[point_a].x = (uint16_t)buf[base] | (uint16_t)((high & 0x30U) << 4U);
            frame->points[point_a].y = y;
            frame->points[point_a].valid = 1U;
            frame->seen_mask |= (uint8_t)(1U << point_a);
        }

        y = (uint16_t)buf[base + 4U] | (uint16_t)((high & 0x0CU) << 6U);
        if (y <= IR_CAMERA_MAX_Y) {
            frame->points[point_b].x = (uint16_t)buf[base + 3U] | (uint16_t)((high & 0x03U) << 8U);
            frame->points[point_b].y = y;
            frame->points[point_b].valid = 1U;
            frame->seen_mask |= (uint8_t)(1U << point_b);
        }
    }
}

static int ir_basic_atomic_read(ir_raw_frame_t *frame)
{
    uint8_t first[IR_BASIC_FRAME_LEN];
    uint8_t second[IR_BASIC_FRAME_LEN];
    uint32_t tries = IR_ATOMIC_RETRY + 1U;

    while (tries > 0U) {
        if (ir_i2c_read_reg(0x36U, first, sizeof(first)) != 0) {
            return -1;
        }
        if (ir_i2c_read_reg(0x36U, second, sizeof(second)) != 0) {
            return -1;
        }
        if (memcmp(&first[1], &second[1], IR_BASIC_FRAME_LEN - 1U) == 0) {
            ir_unpack_basic_frame(first, frame);
            return 0;
        }
        tries--;
        if (tries == 0U) {
            ir_unpack_basic_frame(second, frame);
            return 0;
        }
    }

    return -1;
}

static int ir_pick_lr_pair(const ir_raw_frame_t *frame, uint8_t *left_idx, uint8_t *right_idx)
{
    uint8_t i;
    uint8_t first = 0xFFU;
    uint8_t left = 0xFFU;
    uint8_t right = 0xFFU;

    for (i = 0U; i < IR_POINT_MAX; i++) {
        if (frame->points[i].valid == 0U) {
            continue;
        }
        if (first == 0xFFU) {
            first = i;
            left = i;
            right = i;
            continue;
        }
        if (frame->points[i].x < frame->points[left].x) {
            left = i;
        }
        if (frame->points[i].x > frame->points[right].x) {
            right = i;
        }
    }

    if (first == 0xFFU) {
        return -1;
    }
    *left_idx = left;
    *right_idx = right;
    return 0;
}

static void ir_solve_frame(const ir_raw_frame_t *frame, drv_ir_cam_solution_t *solution)
{
    uint16_t corner_x[IR_POINT_MAX];
    uint16_t corner_y[IR_POINT_MAX];
    uint8_t left_idx = 0U;
    uint8_t right_idx = 0U;
    uint8_t i;
    uint32_t sum_y = 0U;
    uint32_t sum_count = 0U;
    uint16_t center_x;
    uint16_t center_y;

    (void)memset(solution, 0, sizeof(*solution));
    solution->seen_count = ir_popcount4(frame->seen_mask);

    if (solution->seen_count == 0U) {
        return;
    }

    if (solution->seen_count < IR_MIN_POINTS_FOR_SOLVE) {
        solution->degraded = (solution->seen_count > 0U) ? 1U : 0U;
        return;
    }

    if (ir_build_ordered_corners(frame, solution, corner_x, corner_y) != 0) {
        uint32_t sum_x = 0U;
        uint32_t sum_y2 = 0U;
        for (i = 0U; i < IR_POINT_MAX; i++) {
            sum_x += corner_x[i];
            sum_y2 += corner_y[i];
        }
        solution->raw_center_x = (uint16_t)(sum_x / IR_POINT_MAX);
        solution->raw_center_y = (uint16_t)(sum_y2 / IR_POINT_MAX);
        solution->point_spacing = (corner_x[1] >= corner_x[0]) ? (uint16_t)(corner_x[1] - corner_x[0]) : 0U;
        solution->screen_x = ir_map_linear(solution->raw_center_x, IR_CAMERA_MAX_X, IR_SCREEN_MAX_X);
        solution->screen_y = ir_map_linear(solution->raw_center_y, IR_CAMERA_MAX_Y, IR_SCREEN_MAX_Y);
        solution->valid = 1U;
        solution->degraded = (solution->seen_count < 4U) ? 1U : 0U;
        ir_save_ordered_corners(corner_x, corner_y);
        ir_apply_smoothing(solution);
        return;
    }

    if (ir_pick_lr_pair(frame, &left_idx, &right_idx) != 0) {
        return;
    }

    center_x = (uint16_t)(((uint32_t)frame->points[left_idx].x + (uint32_t)frame->points[right_idx].x) / 2U);
    for (i = 0U; i < IR_POINT_MAX; i++) {
        if (frame->points[i].valid == 0U) {
            continue;
        }
        sum_y += frame->points[i].y;
        sum_count++;
    }
    center_y = (sum_count == 0U) ? 0U : (uint16_t)(sum_y / sum_count);

    solution->raw_center_x = center_x;
    solution->raw_center_y = center_y;
    solution->point_spacing = (frame->points[right_idx].x >= frame->points[left_idx].x) ?
        (uint16_t)(frame->points[right_idx].x - frame->points[left_idx].x) : 0U;
    solution->screen_x = ir_map_linear(center_x, IR_CAMERA_MAX_X, IR_SCREEN_MAX_X);
    solution->screen_y = ir_map_linear(center_y, IR_CAMERA_MAX_Y, IR_SCREEN_MAX_Y);
    solution->valid = 1U;
    ir_apply_smoothing(solution);
}

static int ir_sensor_init(void)
{
    uint8_t reg06;
    uint8_t reg08;
    uint8_t reg1a;
    errcode_t ret;

#if defined(CONFIG_PINCTRL_SUPPORT_IE)
    (void)uapi_pin_set_ie((pin_t)OF_IR_SDA_PIN, PIN_IE_1);
#endif
    (void)uapi_pin_set_mode((pin_t)OF_IR_SCL_PIN, (pin_mode_t)OF_IR_SCL_PIN_MODE);
    (void)uapi_pin_set_mode((pin_t)OF_IR_SDA_PIN, (pin_mode_t)OF_IR_SDA_PIN_MODE);

    ret = uapi_i2c_master_init((i2c_bus_t)OF_IR_I2C_BUS, OF_IR_I2C_BAUDRATE, 0U);
    if ((ret != ERRCODE_SUCC) && (ret != ERRCODE_I2C_ALREADY_INIT)) {
        osal_printk("[drv_ir_cam] i2c init failed, ret=%d.\r\n", (int)ret);
        return -1;
    }

    ir_fill_sensitivity((uint8_t)OF_IR_SENSITIVITY, &reg06, &reg08, &reg1a);
    if (ir_write_reg_pair(0x30U, 0x01U) != 0) return -1;
    if (ir_write_reg_pair(0x06U, reg06) != 0) return -1;
    if (ir_write_reg_pair(0x08U, reg08) != 0) return -1;
    if (ir_write_reg_pair(0x1AU, reg1a) != 0) return -1;
    if (ir_write_reg_pair(0x33U, 0x11U) != 0) return -1;
    if (ir_write_reg_pair(0x30U, 0x08U) != 0) return -1;

    osal_msleep(100U);
    return 0;
}

static int ir_open(void *ctx)
{
    (void)ctx;
    g_opened = 1;
    g_ready = (ir_sensor_init() == 0) ? 1 : 0;
    g_first_solution_ready = 0U;
    g_ordered_corners_ready = 0U;
    (void)memset(g_last_corner_x, 0, sizeof(g_last_corner_x));
    (void)memset(g_last_corner_y, 0, sizeof(g_last_corner_y));
    (void)memset(&g_last_solution, 0, sizeof(g_last_solution));
    return (g_ready != 0) ? 0 : -1;
}

static int ir_close(void *ctx)
{
    (void)ctx;
    (void)uapi_i2c_deinit((i2c_bus_t)OF_IR_I2C_BUS);
    g_opened = 0;
    g_ready = 0;
    return 0;
}

static int ir_read(void *ctx, uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    ir_raw_frame_t frame;

    (void)ctx;
    if (out_len != 0) {
        *out_len = 0U;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0) || (len < 5U)) {
        return -1;
    }

    if (ir_basic_atomic_read(&frame) != 0) {
        g_last_solution.valid = 0U;
        g_ready = 0;
    } else {
        g_ready = 1;
        ir_solve_frame(&frame, &g_last_solution);
    }

    buf[0] = (uint8_t)(g_last_solution.screen_x & 0xFFU);
    buf[1] = (uint8_t)((g_last_solution.screen_x >> 8) & 0xFFU);
    buf[2] = (uint8_t)(g_last_solution.screen_y & 0xFFU);
    buf[3] = (uint8_t)((g_last_solution.screen_y >> 8) & 0xFFU);
    buf[4] = g_last_solution.valid ? 1U : 0U;
    *out_len = 5U;
    return 0;
}

static int ir_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    i2c_data_t data;

    (void)ctx;
    if (out_len != 0) {
        *out_len = 0U;
    }
    if ((buf == 0) || (len == 0U) || (out_len == 0)) {
        return -1;
    }

    (void)memset(&data, 0, sizeof(data));
    data.send_buf = (uint8_t *)buf;
    data.send_len = len;
    if (uapi_i2c_master_write((i2c_bus_t)OF_IR_I2C_BUS, OF_IR_I2C_ADDR, &data) == ERRCODE_SUCC) {
        *out_len = len;
        return 0;
    }
    return -1;
}

static int ir_ioctl(void *ctx, uint32_t cmd, void *arg)
{
    unused(ctx);
    unused(cmd);
    unused(arg);
    return 0;
}

static const of_fops_t g_ops = {
    .open = ir_open,
    .close = ir_close,
    .read = ir_read,
    .write = ir_write,
    .ioctl = ir_ioctl,
};

static of_dev_t g_dev = {
    .name = "ir_cam",
    .ops = &g_ops,
    .priv = 0,
};

const of_dev_t *drv_ir_cam_get_dev(void)
{
    return &g_dev;
}

int drv_ir_cam_is_ready(void)
{
    return (g_opened != 0) && (g_ready != 0);
}

int drv_ir_cam_get_latest_solution(drv_ir_cam_solution_t *solution)
{
    if (solution == 0) {
        return -1;
    }
    *solution = g_last_solution;
    return 0;
}
