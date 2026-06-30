#include "ir_test.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common_def.h"
#include "i2c.h"
#include "osal_debug.h"
#include "osal_timer.h"
#include "pinctrl.h"
#include "soc_osal.h"

#define IR_TEST_BASIC_FRAME_LEN 11U
#define IR_TEST_POINT_MAX 4U
#define IR_TEST_CAMERA_MAX_Y 2000U
#define IR_TEST_CAMERA_MAX_X 3200U
#define IR_TEST_INIT_DELAY_MS 10U
#define IR_TEST_BOOT_DELAY_MS 1200U

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t valid;
} ir_test_point_t;

typedef struct {
    ir_test_point_t points[IR_TEST_POINT_MAX];
    uint8_t seen_mask;
    int32_t atomic_status;
    uint8_t basic_buf[IR_TEST_BASIC_FRAME_LEN];
} ir_test_raw_frame_t;

typedef struct {
    uint16_t screen_x;
    uint16_t screen_y;
    uint16_t raw_center_x;
    uint16_t raw_center_y;
    uint16_t point_spacing;
    uint8_t valid;
    uint8_t seen_count;
    uint8_t degraded;
    uint8_t ordered_valid;
    uint8_t ordered_mode;
    uint16_t ordered_x[IR_TEST_POINT_MAX];
    uint16_t ordered_y[IR_TEST_POINT_MAX];
} ir_test_solution_t;

typedef struct {
    osal_timer timer;
    uint8_t timer_ready;
    uint8_t i2c_ready;
    uint8_t first_solution_ready;
    uint8_t ordered_corners_ready;
    uint32_t poll_count;
    uint32_t replay_index;
    uint16_t smooth_x;
    uint16_t smooth_y;
    uint16_t last_corner_x[IR_TEST_POINT_MAX];
    uint16_t last_corner_y[IR_TEST_POINT_MAX];
    uint8_t last_log_valid;
    uint8_t last_log_seen_count;
    uint8_t last_log_degraded;
    uint16_t last_log_screen_x;
    uint16_t last_log_screen_y;
    uint32_t no_valid_streak;
} ir_test_ctx_t;

typedef struct {
    uint16_t x[IR_TEST_POINT_MAX];
    uint16_t y[IR_TEST_POINT_MAX];
    uint8_t mask;
} ir_test_replay_frame_t;

enum {
    IR_TEST_ATOMIC_OK = 0,
    IR_TEST_ATOMIC_MISMATCH_FALLBACK = 1,
    IR_TEST_ATOMIC_I2C_ERROR = -1,
    IR_TEST_ATOMIC_DATA_MISMATCH = -2,
};

enum {
    IR_TEST_ORDER_MODE_NONE = 0,
    IR_TEST_ORDER_MODE_DIRECT4 = 1,
    IR_TEST_ORDER_MODE_RECON3 = 2,
    IR_TEST_ORDER_MODE_TRACK2 = 3,
    IR_TEST_ORDER_MODE_TRACK1 = 4,
};

static ir_test_ctx_t g_ir_test;
static ir_test_runtime_solution_t g_ir_latest_solution;

static const ir_test_replay_frame_t g_ir_replay_frames[] = {
    {{180U, 840U, 0U, 0U}, {210U, 205U, 0U, 0U}, 0x03U},
    {{220U, 880U, 0U, 0U}, {220U, 215U, 0U, 0U}, 0x03U},
    {{300U, 940U, 0U, 0U}, {230U, 225U, 0U, 0U}, 0x03U},
    {{420U, 1000U, 0U, 0U}, {245U, 240U, 0U, 0U}, 0x03U},
    {{0U, 0U, 0U, 0U}, {0U, 0U, 0U, 0U}, 0x00U},
    {{260U, 760U, 0U, 0U}, {250U, 255U, 0U, 0U}, 0x03U},
};

static void ir_test_timer_arm(uint32_t delay_ms)
{
    (void)osal_timer_stop(&g_ir_test.timer);
    (void)osal_timer_mod(&g_ir_test.timer, delay_ms);
}

static uint8_t ir_test_popcount4(uint8_t value)
{
    uint8_t count = 0U;
    uint8_t mask = value & 0x0FU;
    while (mask != 0U) {
        count = (uint8_t)(count + (mask & 0x01U));
        mask >>= 1;
    }
    return count;
}

static int32_t ir_test_i2c_write_bytes(const uint8_t *buf, uint32_t len)
{
    i2c_data_t data;
    errcode_t ret;

    (void)memset(&data, 0, sizeof(data));
    data.send_buf = (uint8_t *)buf;
    data.send_len = len;

    ret = uapi_i2c_master_write((i2c_bus_t)IR_TEST_I2C_BUS_ID, (uint16_t)IR_TEST_I2C_ADDR, &data);
    return (ret == ERRCODE_SUCC) ? 0 : -1;
}

static int32_t ir_test_i2c_read_reg(uint8_t reg, uint8_t *rx, uint32_t rx_len)
{
    i2c_data_t data;
    errcode_t ret;

    (void)memset(&data, 0, sizeof(data));
    data.send_buf = &reg;
    data.send_len = 1U;
    data.receive_buf = rx;
    data.receive_len = rx_len;

    ret = uapi_i2c_master_writeread((i2c_bus_t)IR_TEST_I2C_BUS_ID, (uint16_t)IR_TEST_I2C_ADDR, &data);
    return (ret == ERRCODE_SUCC) ? 0 : -1;
}

static int32_t ir_test_write_reg_pair(uint8_t reg, uint8_t value)
{
    uint8_t tx[2];
    tx[0] = reg;
    tx[1] = value;
    return ir_test_i2c_write_bytes(tx, sizeof(tx));
}

static void ir_test_fill_sensitivity(uint8_t sensitivity, uint8_t *reg06, uint8_t *reg08, uint8_t *reg1a)
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

static int32_t ir_test_sensor_init(void)
{
    uint8_t reg06;
    uint8_t reg08;
    uint8_t reg1a;
    errcode_t ret;

#if defined(CONFIG_PINCTRL_SUPPORT_IE)
    (void)uapi_pin_set_ie((pin_t)IR_TEST_SDA_PIN, PIN_IE_1);
#endif
    (void)uapi_pin_set_mode((pin_t)IR_TEST_SCL_PIN, (pin_mode_t)IR_TEST_SCL_PIN_MODE);
    (void)uapi_pin_set_mode((pin_t)IR_TEST_SDA_PIN, (pin_mode_t)IR_TEST_SDA_PIN_MODE);

    ret = uapi_i2c_master_init((i2c_bus_t)IR_TEST_I2C_BUS_ID, IR_TEST_I2C_BAUDRATE, 0U);
    if ((ret != ERRCODE_SUCC) && (ret != ERRCODE_I2C_ALREADY_INIT)) {
        osal_printk("[ir_test] uapi_i2c_master_init failed, bus=%u ret=%d.\r\n",
            (unsigned int)IR_TEST_I2C_BUS_ID, (int)ret);
        return -1;
    }

    reg06 = 0U;
    reg08 = 0U;
    reg1a = 0U;
    ir_test_fill_sensitivity((uint8_t)IR_TEST_SENSITIVITY, &reg06, &reg08, &reg1a);

    if (ir_test_write_reg_pair(0x30U, 0x01U) != 0) {
        return -1;
    }
    osal_msleep(IR_TEST_INIT_DELAY_MS);
    if (ir_test_write_reg_pair(0x06U, reg06) != 0) {
        return -1;
    }
    osal_msleep(IR_TEST_INIT_DELAY_MS);
    if (ir_test_write_reg_pair(0x08U, reg08) != 0) {
        return -1;
    }
    osal_msleep(IR_TEST_INIT_DELAY_MS);
    if (ir_test_write_reg_pair(0x1AU, reg1a) != 0) {
        return -1;
    }
    osal_msleep(IR_TEST_INIT_DELAY_MS);
    if (ir_test_write_reg_pair(0x33U, 0x11U) != 0) {
        return -1;
    }
    osal_msleep(IR_TEST_INIT_DELAY_MS);
    if (ir_test_write_reg_pair(0x30U, 0x08U) != 0) {
        return -1;
    }

    osal_msleep(100U);
    g_ir_test.i2c_ready = 1U;
    osal_printk("[ir_test] sensor init ok, bus=%u addr=0x%x scl=%u sda=%u sens=%u.\r\n",
        (unsigned int)IR_TEST_I2C_BUS_ID,
        (unsigned int)IR_TEST_I2C_ADDR,
        (unsigned int)IR_TEST_SCL_PIN,
        (unsigned int)IR_TEST_SDA_PIN,
        (unsigned int)IR_TEST_SENSITIVITY);
    return 0;
}

static void ir_test_unpack_basic_frame(const uint8_t *buf, ir_test_raw_frame_t *frame)
{
    uint8_t high;
    uint16_t x;
    uint16_t y;
    uint32_t pair_index;

    (void)memset(frame, 0, sizeof(*frame));
    (void)memcpy(frame->basic_buf, buf, IR_TEST_BASIC_FRAME_LEN);
    for (pair_index = 0U; pair_index < 2U; pair_index++) {
        uint32_t base = 1U + pair_index * 5U;
        uint32_t point_a = pair_index * 2U;
        uint32_t point_b = point_a + 1U;

        high = buf[base + 2U];

        x = (uint16_t)buf[base + 0U] | (uint16_t)((high & 0x30U) << 4U);
        y = (uint16_t)buf[base + 1U] | (uint16_t)((high & 0xC0U) << 2U);
        if ((x < 1023U) && (y < 1023U) && (y <= (uint16_t)IR_TEST_CAM_MAX_Y) && (x <= (uint16_t)IR_TEST_CAM_MAX_X)) {
            frame->points[point_a].x = x;
            frame->points[point_a].y = y;
            frame->points[point_a].valid = 1U;
            frame->seen_mask |= (uint8_t)(1U << point_a);
        }

        x = (uint16_t)buf[base + 3U] | (uint16_t)((high & 0x03U) << 8U);
        y = (uint16_t)buf[base + 4U] | (uint16_t)((high & 0x0CU) << 6U);
        if ((x < 1023U) && (y < 1023U) && (y <= (uint16_t)IR_TEST_CAM_MAX_Y) && (x <= (uint16_t)IR_TEST_CAM_MAX_X)) {
            frame->points[point_b].x = x;
            frame->points[point_b].y = y;
            frame->points[point_b].valid = 1U;
            frame->seen_mask |= (uint8_t)(1U << point_b);
        }
    }
}

static int32_t ir_test_basic_atomic_read(ir_test_raw_frame_t *frame)
{
    uint8_t first[IR_TEST_BASIC_FRAME_LEN];
    uint8_t second[IR_TEST_BASIC_FRAME_LEN];
    uint32_t tries = IR_TEST_ATOMIC_RETRY + 1U;

    while (tries > 0U) {
        if (ir_test_i2c_read_reg(0x36U, first, sizeof(first)) != 0) {
            return IR_TEST_ATOMIC_I2C_ERROR;
        }
        if (ir_test_i2c_read_reg(0x36U, second, sizeof(second)) != 0) {
            return IR_TEST_ATOMIC_I2C_ERROR;
        }

        if (memcmp(&first[1], &second[1], IR_TEST_BASIC_FRAME_LEN - 1U) == 0) {
            ir_test_unpack_basic_frame(first, frame);
            frame->atomic_status = IR_TEST_ATOMIC_OK;
            return IR_TEST_ATOMIC_OK;
        }

        tries--;
        if (tries == 0U) {
            ir_test_unpack_basic_frame(second, frame);
            frame->atomic_status = IR_TEST_ATOMIC_MISMATCH_FALLBACK;
            return IR_TEST_ATOMIC_MISMATCH_FALLBACK;
        }
    }

    return IR_TEST_ATOMIC_DATA_MISMATCH;
}

static uint16_t ir_test_clamp_u16(uint32_t value, uint32_t max_value)
{
    if (value > max_value) {
        return (uint16_t)max_value;
    }
    return (uint16_t)value;
}

static uint16_t ir_test_map_linear(uint16_t input, uint16_t in_min, uint16_t in_max, uint16_t out_max)
{
    uint32_t numerator;
    uint32_t denominator;

    if (in_max <= in_min) {
        return 0U;
    }
    if (input <= in_min) {
        return 0U;
    }
    if (input >= in_max) {
        return out_max;
    }

    numerator = (uint32_t)(input - in_min) * (uint32_t)out_max;
    denominator = (uint32_t)(in_max - in_min);
    return ir_test_clamp_u16(numerator / denominator, out_max);
}

static uint16_t ir_test_map_screen_x(uint16_t raw_x)
{
    uint16_t mapped = ir_test_map_linear(raw_x,
        (uint16_t)IR_TEST_CAM_MIN_X,
        (uint16_t)IR_TEST_CAM_MAX_X,
        (uint16_t)(IR_TEST_SCREEN_WIDTH - 1U));
#if IR_TEST_INVERT_X
    mapped = (uint16_t)((IR_TEST_SCREEN_WIDTH - 1U) - mapped);
#endif
    return mapped;
}

static uint16_t ir_test_map_screen_y(uint16_t raw_y)
{
    uint16_t mapped = ir_test_map_linear(raw_y,
        (uint16_t)IR_TEST_CAM_MIN_Y,
        (uint16_t)IR_TEST_CAM_MAX_Y,
        (uint16_t)(IR_TEST_SCREEN_HEIGHT - 1U));
#if IR_TEST_INVERT_Y
    mapped = (uint16_t)((IR_TEST_SCREEN_HEIGHT - 1U) - mapped);
#endif
    return mapped;
}

static void ir_test_apply_smoothing(ir_test_solution_t *solution)
{
    uint32_t smooth_percent = IR_TEST_SMOOTH_PERCENT;
    uint32_t fresh_percent;

    if (solution->valid == 0U) {
        return;
    }
    if (smooth_percent > 95U) {
        smooth_percent = 95U;
    }
    fresh_percent = 100U - smooth_percent;

    if (g_ir_test.first_solution_ready == 0U || smooth_percent == 0U) {
        g_ir_test.smooth_x = solution->screen_x;
        g_ir_test.smooth_y = solution->screen_y;
        g_ir_test.first_solution_ready = 1U;
    } else {
        g_ir_test.smooth_x = (uint16_t)(((uint32_t)g_ir_test.smooth_x * smooth_percent +
            (uint32_t)solution->screen_x * fresh_percent) / 100U);
        g_ir_test.smooth_y = (uint16_t)(((uint32_t)g_ir_test.smooth_y * smooth_percent +
            (uint32_t)solution->screen_y * fresh_percent) / 100U);
    }

    solution->screen_x = g_ir_test.smooth_x;
    solution->screen_y = g_ir_test.smooth_y;
}

static void ir_test_publish_solution(const ir_test_solution_t *solution)
{
    g_ir_latest_solution.screen_x = solution->screen_x;
    g_ir_latest_solution.screen_y = solution->screen_y;
    g_ir_latest_solution.raw_center_x = solution->raw_center_x;
    g_ir_latest_solution.raw_center_y = solution->raw_center_y;
    g_ir_latest_solution.point_spacing = solution->point_spacing;
    g_ir_latest_solution.valid = solution->valid;
    g_ir_latest_solution.onscreen = solution->valid;
    g_ir_latest_solution.seen_count = solution->seen_count;
    g_ir_latest_solution.degraded = solution->degraded;
}

static uint32_t ir_test_dist_sq_u16(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    int32_t dx = (int32_t)x0 - (int32_t)x1;
    int32_t dy = (int32_t)y0 - (int32_t)y1;
    return (uint32_t)(dx * dx + dy * dy);
}

static uint16_t ir_test_clamp_coord_i32(int32_t value, int32_t max_value)
{
    if (value < 0) {
        return 0U;
    }
    if (value > max_value) {
        return (uint16_t)max_value;
    }
    return (uint16_t)value;
}

static uint32_t ir_test_abs_diff_u16(uint16_t a, uint16_t b)
{
    return (a >= b) ? (uint32_t)(a - b) : (uint32_t)(b - a);
}

static bool ir_test_validate_ordered_corners(const uint16_t *corner_x, const uint16_t *corner_y)
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
    uint32_t top_avg_y;
    uint32_t bottom_avg_y;
    uint32_t left_avg_x;
    uint32_t right_avg_x;

    top_avg_y = ((uint32_t)corner_y[0] + (uint32_t)corner_y[1]) / 2U;
    bottom_avg_y = ((uint32_t)corner_y[2] + (uint32_t)corner_y[3]) / 2U;
    left_avg_x = ((uint32_t)corner_x[0] + (uint32_t)corner_x[2]) / 2U;
    right_avg_x = ((uint32_t)corner_x[1] + (uint32_t)corner_x[3]) / 2U;

    if ((top_avg_y + 8U) >= bottom_avg_y) {
        return false;
    }
    if ((left_avg_x + 8U) >= right_avg_x) {
        return false;
    }

    width_top_sq = ir_test_dist_sq_u16(corner_x[0], corner_y[0], corner_x[1], corner_y[1]);
    width_bottom_sq = ir_test_dist_sq_u16(corner_x[2], corner_y[2], corner_x[3], corner_y[3]);
    height_left_sq = ir_test_dist_sq_u16(corner_x[0], corner_y[0], corner_x[2], corner_y[2]);
    height_right_sq = ir_test_dist_sq_u16(corner_x[1], corner_y[1], corner_x[3], corner_y[3]);

    min_width_sq = (width_top_sq < width_bottom_sq) ? width_top_sq : width_bottom_sq;
    max_width_sq = (width_top_sq > width_bottom_sq) ? width_top_sq : width_bottom_sq;
    min_height_sq = (height_left_sq < height_right_sq) ? height_left_sq : height_right_sq;
    max_height_sq = (height_left_sq > height_right_sq) ? height_left_sq : height_right_sq;

    if (min_width_sq < 400U || min_height_sq < 100U) {
        return false;
    }
    if (max_width_sq > (min_width_sq * 6U)) {
        return false;
    }
    if (max_height_sq > (min_height_sq * 6U)) {
        return false;
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
        return false;
    }

    if (ir_test_abs_diff_u16(corner_x[0], corner_x[2]) > 700U &&
        ir_test_abs_diff_u16(corner_x[1], corner_x[3]) > 700U) {
        return false;
    }

    return true;
}

static void ir_test_order_quad_points(const uint16_t *src_x, const uint16_t *src_y, uint16_t *dst_x, uint16_t *dst_y)
{
    uint8_t used[IR_TEST_POINT_MAX] = {0U};
    uint8_t i;
    uint8_t best_idx;
    int32_t best_metric;
    int32_t metric;

    /* TL: min(x + y) */
    best_idx = 0U;
    best_metric = (int32_t)src_x[0] + (int32_t)src_y[0];
    for (i = 1U; i < IR_TEST_POINT_MAX; i++) {
        metric = (int32_t)src_x[i] + (int32_t)src_y[i];
        if (metric < best_metric) {
            best_metric = metric;
            best_idx = i;
        }
    }
    dst_x[0] = src_x[best_idx];
    dst_y[0] = src_y[best_idx];
    used[best_idx] = 1U;

    /* BR: max(x + y) */
    best_idx = 0U;
    best_metric = -1;
    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
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

    /* TR: max(x - y) */
    best_idx = 0U;
    best_metric = INT32_MIN;
    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
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

    /* BL: remaining point */
    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
        if (used[i] == 0U) {
            dst_x[2] = src_x[i];
            dst_y[2] = src_y[i];
            break;
        }
    }

    /* Final safety normalization to enforce TL,TR,BL,BR topology. */
    if (dst_y[0] > dst_y[2]) {
        uint16_t tx = dst_x[0];
        uint16_t ty = dst_y[0];
        dst_x[0] = dst_x[2];
        dst_y[0] = dst_y[2];
        dst_x[2] = tx;
        dst_y[2] = ty;
    }
    if (dst_y[1] > dst_y[3]) {
        uint16_t tx = dst_x[1];
        uint16_t ty = dst_y[1];
        dst_x[1] = dst_x[3];
        dst_y[1] = dst_y[3];
        dst_x[3] = tx;
        dst_y[3] = ty;
    }
    if (dst_x[0] > dst_x[1]) {
        uint16_t tx = dst_x[0];
        uint16_t ty = dst_y[0];
        dst_x[0] = dst_x[1];
        dst_y[0] = dst_y[1];
        dst_x[1] = tx;
        dst_y[1] = ty;
    }
    if (dst_x[2] > dst_x[3]) {
        uint16_t tx = dst_x[2];
        uint16_t ty = dst_y[2];
        dst_x[2] = dst_x[3];
        dst_y[2] = dst_y[3];
        dst_x[3] = tx;
        dst_y[3] = ty;
    }
}

static void ir_test_save_ordered_corners(const uint16_t *corner_x, const uint16_t *corner_y)
{
    uint8_t i;

    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
        g_ir_test.last_corner_x[i] = corner_x[i];
        g_ir_test.last_corner_y[i] = corner_y[i];
    }
    g_ir_test.ordered_corners_ready = 1U;
}

static bool ir_test_try_track_single_from_previous(const ir_test_raw_frame_t *frame, uint16_t *corner_x, uint16_t *corner_y)
{
    uint8_t i;
    uint8_t valid_idx = 0xFFU;
    uint8_t best_corner = 0U;
    uint32_t best_cost = UINT32_MAX;
    int32_t dx;
    int32_t dy;

    if (g_ir_test.ordered_corners_ready == 0U) {
        return false;
    }

    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
        if (frame->points[i].valid != 0U) {
            valid_idx = i;
            break;
        }
    }
    if (valid_idx == 0xFFU) {
        return false;
    }

    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
        uint32_t cost = ir_test_dist_sq_u16(frame->points[valid_idx].x, frame->points[valid_idx].y,
            g_ir_test.last_corner_x[i], g_ir_test.last_corner_y[i]);
        if (cost < best_cost) {
            best_cost = cost;
            best_corner = i;
        }
    }

    dx = (int32_t)frame->points[valid_idx].x - (int32_t)g_ir_test.last_corner_x[best_corner];
    dy = (int32_t)frame->points[valid_idx].y - (int32_t)g_ir_test.last_corner_y[best_corner];

    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
        int32_t tx = (int32_t)g_ir_test.last_corner_x[i] + dx;
        int32_t ty = (int32_t)g_ir_test.last_corner_y[i] + dy;
        corner_x[i] = ir_test_clamp_coord_i32(tx, (int32_t)IR_TEST_CAMERA_MAX_X);
        corner_y[i] = ir_test_clamp_coord_i32(ty, (int32_t)IR_TEST_CAMERA_MAX_Y);
    }
    corner_x[best_corner] = frame->points[valid_idx].x;
    corner_y[best_corner] = frame->points[valid_idx].y;
    return true;
}

static bool ir_test_try_track_pair_from_previous(const ir_test_raw_frame_t *frame, uint16_t *corner_x, uint16_t *corner_y)
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

    if (g_ir_test.ordered_corners_ready == 0U) {
        return false;
    }

    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
        if (frame->points[i].valid == 0U) {
            continue;
        }
        if (vis_count < 2U) {
            vis_idx[vis_count] = i;
        }
        vis_count++;
    }
    if (vis_count != 2U) {
        return false;
    }

    for (a = 0U; a < IR_TEST_POINT_MAX; a++) {
        for (b = 0U; b < IR_TEST_POINT_MAX; b++) {
            uint32_t cost_direct;
            uint32_t cost_swap;

            if (a == b) {
                continue;
            }

            cost_direct =
                ir_test_dist_sq_u16(frame->points[vis_idx[0]].x, frame->points[vis_idx[0]].y,
                    g_ir_test.last_corner_x[a], g_ir_test.last_corner_y[a]) +
                ir_test_dist_sq_u16(frame->points[vis_idx[1]].x, frame->points[vis_idx[1]].y,
                    g_ir_test.last_corner_x[b], g_ir_test.last_corner_y[b]);
            if (cost_direct < best_cost) {
                best_cost = cost_direct;
                best_a = a;
                best_b = b;
                best_swap = 0U;
            }

            cost_swap =
                ir_test_dist_sq_u16(frame->points[vis_idx[0]].x, frame->points[vis_idx[0]].y,
                    g_ir_test.last_corner_x[b], g_ir_test.last_corner_y[b]) +
                ir_test_dist_sq_u16(frame->points[vis_idx[1]].x, frame->points[vis_idx[1]].y,
                    g_ir_test.last_corner_x[a], g_ir_test.last_corner_y[a]);
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

    dx0 = (int32_t)frame->points[vis_idx[0]].x - (int32_t)g_ir_test.last_corner_x[best_a];
    dy0 = (int32_t)frame->points[vis_idx[0]].y - (int32_t)g_ir_test.last_corner_y[best_a];
    dx1 = (int32_t)frame->points[vis_idx[1]].x - (int32_t)g_ir_test.last_corner_x[best_b];
    dy1 = (int32_t)frame->points[vis_idx[1]].y - (int32_t)g_ir_test.last_corner_y[best_b];
    avg_dx = (dx0 + dx1) / 2;
    avg_dy = (dy0 + dy1) / 2;

    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
        int32_t tx = (int32_t)g_ir_test.last_corner_x[i] + avg_dx;
        int32_t ty = (int32_t)g_ir_test.last_corner_y[i] + avg_dy;
        corner_x[i] = ir_test_clamp_coord_i32(tx, (int32_t)IR_TEST_CAMERA_MAX_X);
        corner_y[i] = ir_test_clamp_coord_i32(ty, (int32_t)IR_TEST_CAMERA_MAX_Y);
    }

    corner_x[best_a] = frame->points[vis_idx[0]].x;
    corner_y[best_a] = frame->points[vis_idx[0]].y;
    corner_x[best_b] = frame->points[vis_idx[1]].x;
    corner_y[best_b] = frame->points[vis_idx[1]].y;
    return true;
}

static bool ir_test_reconstruct_three_points(const ir_test_raw_frame_t *frame, uint16_t *corner_x, uint16_t *corner_y)
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

    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
        if (frame->points[i].valid == 0U) {
            continue;
        }
        px[count] = frame->points[i].x;
        py[count] = frame->points[i].y;
        count++;
    }
    if (count != 3U) {
        return false;
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
        a_idx = 0U;
        c_idx = 1U;
        b_idx = 2U;
    } else if (d12_sq >= d02_sq) {
        a_idx = 1U;
        c_idx = 2U;
        b_idx = 0U;
    } else {
        a_idx = 0U;
        c_idx = 2U;
        b_idx = 1U;
    }

    {
        int32_t rx = (int32_t)px[a_idx] + (int32_t)px[c_idx] - (int32_t)px[b_idx];
        int32_t ry = (int32_t)py[a_idx] + (int32_t)py[c_idx] - (int32_t)py[b_idx];
        px[3] = ir_test_clamp_coord_i32(rx, (int32_t)IR_TEST_CAMERA_MAX_X);
        py[3] = ir_test_clamp_coord_i32(ry, (int32_t)IR_TEST_CAMERA_MAX_Y);
    }
    ir_test_order_quad_points(px, py, corner_x, corner_y);
    return true;
}

static bool ir_test_build_ordered_corners(const ir_test_raw_frame_t *frame, ir_test_solution_t *solution,
    uint16_t *corner_x, uint16_t *corner_y)
{
    uint16_t px[4];
    uint16_t py[4];
    uint8_t i;
    uint8_t count = 0U;

    if (solution->seen_count == 4U) {
        for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
            if (frame->points[i].valid == 0U) {
                continue;
            }
            px[count] = frame->points[i].x;
            py[count] = frame->points[i].y;
            count++;
        }
        if (count != 4U) {
            return false;
        }
        ir_test_order_quad_points(px, py, corner_x, corner_y);
        if (!ir_test_validate_ordered_corners(corner_x, corner_y)) {
            return false;
        }
        solution->ordered_mode = IR_TEST_ORDER_MODE_DIRECT4;
        return true;
    }

    if (solution->seen_count == 3U) {
        if (ir_test_reconstruct_three_points(frame, corner_x, corner_y)) {
            if (!ir_test_validate_ordered_corners(corner_x, corner_y)) {
                return false;
            }
            solution->ordered_mode = IR_TEST_ORDER_MODE_RECON3;
            return true;
        }
        return false;
    }

    if (solution->seen_count == 2U) {
        if (ir_test_try_track_pair_from_previous(frame, corner_x, corner_y)) {
            if (!ir_test_validate_ordered_corners(corner_x, corner_y)) {
                return false;
            }
            solution->ordered_mode = IR_TEST_ORDER_MODE_TRACK2;
            return true;
        }
        return false;
    }

#if IR_TEST_ENABLE_SINGLE_POINT_DEGRADE
    if (solution->seen_count == 1U) {
        if (ir_test_try_track_single_from_previous(frame, corner_x, corner_y)) {
            if (!ir_test_validate_ordered_corners(corner_x, corner_y)) {
                return false;
            }
            solution->ordered_mode = IR_TEST_ORDER_MODE_TRACK1;
            return true;
        }
        return false;
    }
#endif

    return false;
}

static bool ir_test_pick_lr_pair(const ir_test_raw_frame_t *frame, uint8_t *left_idx, uint8_t *right_idx)
{
    uint8_t i;
    uint8_t first = 0xFFU;
    uint8_t left = 0xFFU;
    uint8_t right = 0xFFU;

    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
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
        return false;
    }
    *left_idx = left;
    *right_idx = right;
    return true;
}

static void ir_test_solve_frame(const ir_test_raw_frame_t *frame, ir_test_solution_t *solution)
{
    uint16_t corner_x[IR_TEST_POINT_MAX];
    uint16_t corner_y[IR_TEST_POINT_MAX];
    uint8_t left_idx = 0U;
    uint8_t right_idx = 0U;
    uint8_t i;
    uint32_t sum_y = 0U;
    uint32_t sum_count = 0U;
    uint16_t center_x;
    uint16_t center_y;

    (void)memset(solution, 0, sizeof(*solution));
    solution->seen_count = ir_test_popcount4(frame->seen_mask);

    if (solution->seen_count < IR_TEST_MIN_POINTS_FOR_SOLVE) {
#if IR_TEST_ENABLE_SINGLE_POINT_DEGRADE
        if (solution->seen_count == 1U) {
            if (ir_test_build_ordered_corners(frame, solution, corner_x, corner_y)) {
                solution->degraded = 1U;
                solution->ordered_valid = 1U;
                for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
                    solution->ordered_x[i] = corner_x[i];
                    solution->ordered_y[i] = corner_y[i];
                }
            }

            for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
                if (frame->points[i].valid == 0U) {
                    continue;
                }
                solution->raw_center_x = frame->points[i].x;
                solution->raw_center_y = frame->points[i].y;
                solution->screen_x = ir_test_map_screen_x(frame->points[i].x);
                solution->screen_y = ir_test_map_screen_y(frame->points[i].y);
                solution->degraded = 1U;
                return;
            }
        }
#endif
        solution->valid = 0U;
        solution->degraded = (solution->seen_count > 0U) ? 1U : 0U;
        return;
    }

    if (ir_test_build_ordered_corners(frame, solution, corner_x, corner_y)) {
        uint32_t sum_x = 0U;
        uint32_t sum_y = 0U;

        for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
            sum_x += corner_x[i];
            sum_y += corner_y[i];
        }

        solution->raw_center_x = (uint16_t)(sum_x / IR_TEST_POINT_MAX);
        solution->raw_center_y = (uint16_t)(sum_y / IR_TEST_POINT_MAX);
        solution->point_spacing = (corner_x[1] >= corner_x[0]) ? (uint16_t)(corner_x[1] - corner_x[0]) : 0U;
        solution->screen_x = ir_test_map_screen_x(solution->raw_center_x);
        solution->screen_y = ir_test_map_screen_y(solution->raw_center_y);
        solution->valid = 1U;
        solution->degraded = (solution->seen_count < 4U) ? 1U : 0U;
        solution->ordered_valid = 1U;
        for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
            solution->ordered_x[i] = corner_x[i];
            solution->ordered_y[i] = corner_y[i];
        }
        ir_test_save_ordered_corners(corner_x, corner_y);
        ir_test_apply_smoothing(solution);
        return;
    }

    if (!ir_test_pick_lr_pair(frame, &left_idx, &right_idx)) {
        return;
    }

    center_x = (uint16_t)(((uint32_t)frame->points[left_idx].x + (uint32_t)frame->points[right_idx].x) / 2U);
    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
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
    solution->screen_x = ir_test_map_screen_x(center_x);
    solution->screen_y = ir_test_map_screen_y(center_y);
    solution->valid = 1U;
    solution->degraded = 1U;
    ir_test_apply_smoothing(solution);
}

static const char *ir_test_order_mode_name(uint8_t mode)
{
    switch (mode) {
        case IR_TEST_ORDER_MODE_DIRECT4:
            return "4pt";
        case IR_TEST_ORDER_MODE_RECON3:
            return "3pt-recon";
        case IR_TEST_ORDER_MODE_TRACK2:
            return "2pt-track";
        case IR_TEST_ORDER_MODE_TRACK1:
            return "1pt-track";
        default:
            return "none";
    }
}

static void ir_test_log_geometry_debug(const ir_test_raw_frame_t *frame, const ir_test_solution_t *solution)
{
    if (solution->ordered_valid == 0U) {
        osal_printk("[ir_test] geom mode=%s ordered=none raw p0=(%u,%u,%u) p1=(%u,%u,%u) p2=(%u,%u,%u) p3=(%u,%u,%u)\r\n",
            ir_test_order_mode_name(solution->ordered_mode),
            (unsigned int)frame->points[0].x, (unsigned int)frame->points[0].y, (unsigned int)frame->points[0].valid,
            (unsigned int)frame->points[1].x, (unsigned int)frame->points[1].y, (unsigned int)frame->points[1].valid,
            (unsigned int)frame->points[2].x, (unsigned int)frame->points[2].y, (unsigned int)frame->points[2].valid,
            (unsigned int)frame->points[3].x, (unsigned int)frame->points[3].y, (unsigned int)frame->points[3].valid);
        return;
    }

    osal_printk("[ir_test] geom mode=%s raw p0=(%u,%u,%u) p1=(%u,%u,%u) p2=(%u,%u,%u) p3=(%u,%u,%u) TL=(%u,%u) TR=(%u,%u) BL=(%u,%u) BR=(%u,%u)\r\n",
        ir_test_order_mode_name(solution->ordered_mode),
        (unsigned int)frame->points[0].x, (unsigned int)frame->points[0].y, (unsigned int)frame->points[0].valid,
        (unsigned int)frame->points[1].x, (unsigned int)frame->points[1].y, (unsigned int)frame->points[1].valid,
        (unsigned int)frame->points[2].x, (unsigned int)frame->points[2].y, (unsigned int)frame->points[2].valid,
        (unsigned int)frame->points[3].x, (unsigned int)frame->points[3].y, (unsigned int)frame->points[3].valid,
        (unsigned int)solution->ordered_x[0], (unsigned int)solution->ordered_y[0],
        (unsigned int)solution->ordered_x[1], (unsigned int)solution->ordered_y[1],
        (unsigned int)solution->ordered_x[2], (unsigned int)solution->ordered_y[2],
        (unsigned int)solution->ordered_x[3], (unsigned int)solution->ordered_y[3]);
}

static void ir_test_fill_replay_frame(ir_test_raw_frame_t *frame)
{
    const ir_test_replay_frame_t *src;
    uint8_t i;

    src = &g_ir_replay_frames[g_ir_test.replay_index % (sizeof(g_ir_replay_frames) / sizeof(g_ir_replay_frames[0]))];
    (void)memset(frame, 0, sizeof(*frame));
    frame->seen_mask = src->mask;
    frame->atomic_status = IR_TEST_ATOMIC_OK;
    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
        if ((src->mask & (1U << i)) == 0U) {
            continue;
        }
        frame->points[i].x = src->x[i];
        frame->points[i].y = src->y[i];
        frame->points[i].valid = 1U;
    }
    g_ir_test.replay_index++;
}

static void ir_test_log_raw_frame(const ir_test_raw_frame_t *frame)
{
    osal_printk("[ir_test] raw poll=%u seen=0x%x atomic=%d p0=(%u,%u,%u) p1=(%u,%u,%u) p2=(%u,%u,%u) p3=(%u,%u,%u)\r\n",
        (unsigned int)g_ir_test.poll_count,
        (unsigned int)frame->seen_mask,
        (int)frame->atomic_status,
        (unsigned int)frame->points[0].x, (unsigned int)frame->points[0].y, (unsigned int)frame->points[0].valid,
        (unsigned int)frame->points[1].x, (unsigned int)frame->points[1].y, (unsigned int)frame->points[1].valid,
        (unsigned int)frame->points[2].x, (unsigned int)frame->points[2].y, (unsigned int)frame->points[2].valid,
        (unsigned int)frame->points[3].x, (unsigned int)frame->points[3].y, (unsigned int)frame->points[3].valid);
}

static void ir_test_log_basic_frame_hex(const ir_test_raw_frame_t *frame, const char *reason)
{
    osal_printk("[ir_test] basic hex %s: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
        reason,
        (unsigned int)frame->basic_buf[0], (unsigned int)frame->basic_buf[1], (unsigned int)frame->basic_buf[2],
        (unsigned int)frame->basic_buf[3], (unsigned int)frame->basic_buf[4], (unsigned int)frame->basic_buf[5],
        (unsigned int)frame->basic_buf[6], (unsigned int)frame->basic_buf[7], (unsigned int)frame->basic_buf[8],
        (unsigned int)frame->basic_buf[9], (unsigned int)frame->basic_buf[10]);
}

static bool ir_test_frame_has_all_max_pattern(const ir_test_raw_frame_t *frame)
{
    uint8_t i;

    for (i = 0U; i < IR_TEST_POINT_MAX; i++) {
        if ((frame->points[i].x != 0U) || (frame->points[i].y != 0U) || (frame->points[i].valid != 0U)) {
            return false;
        }
    }
    return true;
}

static bool ir_test_should_log_no_valid_frame(void)
{
    g_ir_test.no_valid_streak++;
    if (g_ir_test.no_valid_streak <= 3U) {
        return true;
    }
    if ((g_ir_test.no_valid_streak % 25U) == 0U) {
        return true;
    }
    return false;
}

static bool ir_test_should_log_compact(const ir_test_solution_t *solution)
{
    uint32_t log_every = IR_TEST_LOG_EVERY_N;

#if IR_TEST_ENABLE_USB_LIVE_MOUSE_MODE
    log_every = 5000U / IR_TEST_POLL_MS;
#endif

    if (log_every == 0U) {
        log_every = 1U;
    }
    if ((g_ir_test.poll_count % log_every) == 0U) {
        return true;
    }
    if (g_ir_test.last_log_valid != solution->valid) {
        return true;
    }
    if (g_ir_test.last_log_seen_count != solution->seen_count) {
        return true;
    }
    if (g_ir_test.last_log_degraded != solution->degraded) {
        return true;
    }
    if (solution->valid != 0U) {
        int32_t dx = (int32_t)solution->screen_x - (int32_t)g_ir_test.last_log_screen_x;
        int32_t dy = (int32_t)solution->screen_y - (int32_t)g_ir_test.last_log_screen_y;
        if (dx < 0) {
            dx = -dx;
        }
        if (dy < 0) {
            dy = -dy;
        }
        if (dx >= 40 || dy >= 40) {
            return true;
        }
    }
    return false;
}

static void ir_test_log_solution(const ir_test_raw_frame_t *frame, const ir_test_solution_t *solution, const char *source)
{
#if IR_TEST_ENABLE_COMPACT_LOG
    if (!ir_test_should_log_compact(solution)) {
        g_ir_test.last_log_valid = solution->valid;
        g_ir_test.last_log_seen_count = solution->seen_count;
        g_ir_test.last_log_degraded = solution->degraded;
        g_ir_test.last_log_screen_x = solution->screen_x;
        g_ir_test.last_log_screen_y = solution->screen_y;
        return;
    }
#endif
    if (solution->valid == 0U) {
        osal_printk("[ir_test] %s solve invalid, seen=%u mask=0x%x degraded=%u.\r\n",
            source,
            (unsigned int)solution->seen_count,
            (unsigned int)frame->seen_mask,
            (unsigned int)solution->degraded);
        if (solution->seen_count > 0U) {
            ir_test_log_geometry_debug(frame, solution);
        }
        g_ir_test.last_log_valid = solution->valid;
        g_ir_test.last_log_seen_count = solution->seen_count;
        g_ir_test.last_log_degraded = solution->degraded;
        g_ir_test.last_log_screen_x = solution->screen_x;
        g_ir_test.last_log_screen_y = solution->screen_y;
        return;
    }

    osal_printk("[ir_test] %s solve ok, seen=%u center=(%u,%u) spacing=%u screen=(%u,%u).\r\n",
        source,
        (unsigned int)solution->seen_count,
        (unsigned int)solution->raw_center_x,
        (unsigned int)solution->raw_center_y,
        (unsigned int)solution->point_spacing,
        (unsigned int)solution->screen_x,
        (unsigned int)solution->screen_y);
    ir_test_log_geometry_debug(frame, solution);

    g_ir_test.last_log_valid = solution->valid;
    g_ir_test.last_log_seen_count = solution->seen_count;
    g_ir_test.last_log_degraded = solution->degraded;
    g_ir_test.last_log_screen_x = solution->screen_x;
    g_ir_test.last_log_screen_y = solution->screen_y;
}

static void __attribute__((unused)) ir_test_run_live_once(void)
{
    ir_test_raw_frame_t frame;
    ir_test_solution_t solution;
    int32_t ret;

    if (g_ir_test.i2c_ready == 0U) {
        osal_printk("[ir_test] skip poll because sensor not ready.\r\n");
        return;
    }

    ret = ir_test_basic_atomic_read(&frame);
    g_ir_test.poll_count++;
    if (ret == IR_TEST_ATOMIC_I2C_ERROR) {
        osal_printk("[ir_test] basicAtomic read failed on poll=%u.\r\n", (unsigned int)g_ir_test.poll_count);
        return;
    }
    if (ret == IR_TEST_ATOMIC_MISMATCH_FALLBACK) {
        ir_test_log_basic_frame_hex(&frame, "atomic-fallback");
    }
    if (ir_test_frame_has_all_max_pattern(&frame)) {
        if (ir_test_should_log_no_valid_frame()) {
            ir_test_log_basic_frame_hex(&frame, "no-valid-points");
        }
    } else {
        g_ir_test.no_valid_streak = 0U;
    }

#if IR_TEST_CASE == IR_TEST_CASE_RAW_STREAM
    ir_test_log_raw_frame(&frame);
#else
    ir_test_solve_frame(&frame, &solution);
    ir_test_publish_solution(&solution);
    ir_test_log_solution(&frame, &solution, "live");
#endif
}

static void __attribute__((unused)) ir_test_run_replay_once(void)
{
    ir_test_raw_frame_t frame;
    ir_test_solution_t solution;

    ir_test_fill_replay_frame(&frame);
    g_ir_test.poll_count++;
    ir_test_log_raw_frame(&frame);
    ir_test_solve_frame(&frame, &solution);
    ir_test_publish_solution(&solution);
    ir_test_log_solution(&frame, &solution, "replay");
}

static void ir_test_timer_cb(unsigned long arg)
{
    unused(arg);

#if IR_TEST_CASE == IR_TEST_CASE_INIT_ONLY
    osal_printk("[ir_test] init-only mode, sensor_ready=%u.\r\n", (unsigned int)g_ir_test.i2c_ready);
    return;
#elif IR_TEST_CASE == IR_TEST_CASE_SOLVE_REPLAY
    ir_test_run_replay_once();
    ir_test_timer_arm(IR_TEST_REPLAY_STEP_MS);
#else
    ir_test_run_live_once();
    ir_test_timer_arm(IR_TEST_POLL_MS);
#endif
}

static void ir_test_timer_init(void)
{
    g_ir_test.timer.timer = NULL;
    g_ir_test.timer.handler = ir_test_timer_cb;
    g_ir_test.timer.data = 0UL;
    g_ir_test.timer.interval = IR_TEST_BOOT_DELAY_MS;
    if (osal_timer_init(&g_ir_test.timer) != OSAL_SUCCESS) {
        osal_printk("[ir_test] timer init failed.\r\n");
        return;
    }
    g_ir_test.timer_ready = 1U;
}

void ir_test_overlay_entry(void)
{
    (void)memset(&g_ir_test, 0, sizeof(g_ir_test));
    (void)memset(&g_ir_latest_solution, 0, sizeof(g_ir_latest_solution));

    osal_printk("[ir_test] boot, case=%u bus=%u addr=0x%x scl=%u sda=%u baud=%u.\r\n",
        (unsigned int)IR_TEST_CASE,
        (unsigned int)IR_TEST_I2C_BUS_ID,
        (unsigned int)IR_TEST_I2C_ADDR,
        (unsigned int)IR_TEST_SCL_PIN,
        (unsigned int)IR_TEST_SDA_PIN,
        (unsigned int)IR_TEST_I2C_BAUDRATE);
    osal_printk("[ir_test] note: if you use bare Wii camera, external 24/25MHz clock still needs hardware support.\r\n");

#if IR_TEST_CASE != IR_TEST_CASE_SOLVE_REPLAY
    if (ir_test_sensor_init() != 0) {
        osal_printk("[ir_test] sensor init failed, please check i2c wiring / power / external camera clock.\r\n");
    }
#else
    osal_printk("[ir_test] replay mode enabled, solver will run without real sensor input.\r\n");
#endif

    ir_test_timer_init();
    if (g_ir_test.timer_ready == 0U) {
        return;
    }

#if IR_TEST_CASE == IR_TEST_CASE_INIT_ONLY
    ir_test_timer_arm(IR_TEST_BOOT_DELAY_MS);
#elif IR_TEST_CASE == IR_TEST_CASE_SOLVE_REPLAY
    ir_test_timer_arm(IR_TEST_BOOT_DELAY_MS);
#else
    if (g_ir_test.i2c_ready != 0U) {
        ir_test_timer_arm(IR_TEST_BOOT_DELAY_MS);
    }
#endif
}

int ir_test_get_latest_solution(ir_test_runtime_solution_t *solution)
{
    if (solution == NULL) {
        return -1;
    }
    *solution = g_ir_latest_solution;
    return 0;
}
