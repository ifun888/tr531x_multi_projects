#ifndef LIGHT_GUN_DRIVER_TEST_IR_TEST_H
#define LIGHT_GUN_DRIVER_TEST_IR_TEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * IR 传感器测试参数说明：
 *
 * 这套测试不是直接把 OpenFIRE 全量几何算法整包搬过来，
 * 而是先做一版“方便二次开发和闭环验证”的最小可用版本：
 *
 * 1. 初始化 Wii IR Camera 兼容模块
 * 2. 用 basic frame 做双帧稳定读取
 * 3. 解出原始 4 点和 seen mask
 * 4. 优先按 OpenFIRE square 思路识别 TL/TR/BL/BR 四角
 * 5. 缺点时先尝试基于上一帧四角做退化跟踪
 * 6. 用四角中心 + 线性映射得到屏幕坐标
 * 7. 用软件定时器持续跑日志
 *
 * 这样你可以先把：
 * - I2C 通不通
 * - 相机是否有点
 * - seen mask 是否稳定
 * - 坐标解算方向是否正确
 * - 缺点时四角身份是否还能维持
 * 这几个基础问题先验证完。
 */

#ifndef IR_TEST_I2C_BUS_ID
#ifdef CONFIG_LIGHT_GUN_IR_I2C_BUS_ID
#define IR_TEST_I2C_BUS_ID CONFIG_LIGHT_GUN_IR_I2C_BUS_ID
#else
#define IR_TEST_I2C_BUS_ID 0
#endif
#endif

#ifndef IR_TEST_SCL_PIN
#ifdef CONFIG_LIGHT_GUN_IR_SCL_PIN
#define IR_TEST_SCL_PIN CONFIG_LIGHT_GUN_IR_SCL_PIN
#else
#define IR_TEST_SCL_PIN 25
#endif
#endif

#ifndef IR_TEST_SDA_PIN
#ifdef CONFIG_LIGHT_GUN_IR_SDA_PIN
#define IR_TEST_SDA_PIN CONFIG_LIGHT_GUN_IR_SDA_PIN
#else
#define IR_TEST_SDA_PIN 26
#endif
#endif

#ifndef IR_TEST_SCL_PIN_MODE
#ifdef CONFIG_LIGHT_GUN_IR_SCL_PIN_MODE
#define IR_TEST_SCL_PIN_MODE CONFIG_LIGHT_GUN_IR_SCL_PIN_MODE
#else
#define IR_TEST_SCL_PIN_MODE 26
#endif
#endif

#ifndef IR_TEST_SDA_PIN_MODE
#ifdef CONFIG_LIGHT_GUN_IR_SDA_PIN_MODE
#define IR_TEST_SDA_PIN_MODE CONFIG_LIGHT_GUN_IR_SDA_PIN_MODE
#else
#define IR_TEST_SDA_PIN_MODE 27
#endif
#endif

#ifndef IR_TEST_I2C_ADDR
#ifdef CONFIG_LIGHT_GUN_IR_I2C_ADDR
#define IR_TEST_I2C_ADDR CONFIG_LIGHT_GUN_IR_I2C_ADDR
#else
#define IR_TEST_I2C_ADDR 0x58
#endif
#endif

#ifndef IR_TEST_I2C_BAUDRATE
#ifdef CONFIG_LIGHT_GUN_IR_I2C_BAUDRATE
#define IR_TEST_I2C_BAUDRATE CONFIG_LIGHT_GUN_IR_I2C_BAUDRATE
#else
#define IR_TEST_I2C_BAUDRATE 200000U
#endif
#endif

#ifndef IR_TEST_POLL_MS
#ifdef CONFIG_LIGHT_GUN_IR_POLL_MS
#define IR_TEST_POLL_MS CONFIG_LIGHT_GUN_IR_POLL_MS
#else
#define IR_TEST_POLL_MS 20U
#endif
#endif

#ifndef IR_TEST_REPLAY_STEP_MS
#ifdef CONFIG_LIGHT_GUN_IR_REPLAY_STEP_MS
#define IR_TEST_REPLAY_STEP_MS CONFIG_LIGHT_GUN_IR_REPLAY_STEP_MS
#else
#define IR_TEST_REPLAY_STEP_MS 300U
#endif
#endif

#ifndef IR_TEST_ATOMIC_RETRY
#ifdef CONFIG_LIGHT_GUN_IR_ATOMIC_RETRY
#define IR_TEST_ATOMIC_RETRY CONFIG_LIGHT_GUN_IR_ATOMIC_RETRY
#else
#define IR_TEST_ATOMIC_RETRY 2U
#endif
#endif

#ifndef IR_TEST_SENSITIVITY
#ifdef CONFIG_LIGHT_GUN_IR_SENSITIVITY
#define IR_TEST_SENSITIVITY CONFIG_LIGHT_GUN_IR_SENSITIVITY
#else
#define IR_TEST_SENSITIVITY 0U
#endif
#endif

#define IR_TEST_CASE_INIT_ONLY   1
#define IR_TEST_CASE_RAW_STREAM  2
#define IR_TEST_CASE_SOLVE_LIVE  3
#define IR_TEST_CASE_SOLVE_REPLAY 4

#ifndef IR_TEST_CASE
#if defined(CONFIG_LIGHT_GUN_IR_CASE_INIT_ONLY)
#define IR_TEST_CASE IR_TEST_CASE_INIT_ONLY
#elif defined(CONFIG_LIGHT_GUN_IR_CASE_RAW_STREAM)
#define IR_TEST_CASE IR_TEST_CASE_RAW_STREAM
#elif defined(CONFIG_LIGHT_GUN_IR_CASE_SOLVE_REPLAY)
#define IR_TEST_CASE IR_TEST_CASE_SOLVE_REPLAY
#else
#define IR_TEST_CASE IR_TEST_CASE_SOLVE_LIVE
#endif
#endif

#ifndef IR_TEST_SCREEN_WIDTH
#ifdef CONFIG_LIGHT_GUN_IR_SCREEN_WIDTH
#define IR_TEST_SCREEN_WIDTH CONFIG_LIGHT_GUN_IR_SCREEN_WIDTH
#else
#define IR_TEST_SCREEN_WIDTH 1920U
#endif
#endif

#ifndef IR_TEST_SCREEN_HEIGHT
#ifdef CONFIG_LIGHT_GUN_IR_SCREEN_HEIGHT
#define IR_TEST_SCREEN_HEIGHT CONFIG_LIGHT_GUN_IR_SCREEN_HEIGHT
#else
#define IR_TEST_SCREEN_HEIGHT 1080U
#endif
#endif

#if defined(CONFIG_LIGHT_GUN_IR_INVERT_X)
#define IR_TEST_INVERT_X 1
#else
#define IR_TEST_INVERT_X 0
#endif

#if defined(CONFIG_LIGHT_GUN_IR_INVERT_Y)
#define IR_TEST_INVERT_Y 1
#else
#define IR_TEST_INVERT_Y 0
#endif

#ifndef IR_TEST_CAM_MIN_X
#ifdef CONFIG_LIGHT_GUN_IR_CAM_MIN_X
#define IR_TEST_CAM_MIN_X CONFIG_LIGHT_GUN_IR_CAM_MIN_X
#else
#define IR_TEST_CAM_MIN_X 0U
#endif
#endif

#ifndef IR_TEST_CAM_MAX_X
#ifdef CONFIG_LIGHT_GUN_IR_CAM_MAX_X
#define IR_TEST_CAM_MAX_X CONFIG_LIGHT_GUN_IR_CAM_MAX_X
#else
#define IR_TEST_CAM_MAX_X 1023U
#endif
#endif

#ifndef IR_TEST_CAM_MIN_Y
#ifdef CONFIG_LIGHT_GUN_IR_CAM_MIN_Y
#define IR_TEST_CAM_MIN_Y CONFIG_LIGHT_GUN_IR_CAM_MIN_Y
#else
#define IR_TEST_CAM_MIN_Y 0U
#endif
#endif

#ifndef IR_TEST_CAM_MAX_Y
#ifdef CONFIG_LIGHT_GUN_IR_CAM_MAX_Y
#define IR_TEST_CAM_MAX_Y CONFIG_LIGHT_GUN_IR_CAM_MAX_Y
#else
#define IR_TEST_CAM_MAX_Y 767U
#endif
#endif

#ifndef IR_TEST_SMOOTH_PERCENT
#ifdef CONFIG_LIGHT_GUN_IR_SMOOTH_PERCENT
#define IR_TEST_SMOOTH_PERCENT CONFIG_LIGHT_GUN_IR_SMOOTH_PERCENT
#else
#define IR_TEST_SMOOTH_PERCENT 35U
#endif
#endif

#ifndef IR_TEST_MIN_POINTS_FOR_SOLVE
#ifdef CONFIG_LIGHT_GUN_IR_MIN_POINTS_FOR_SOLVE
#define IR_TEST_MIN_POINTS_FOR_SOLVE CONFIG_LIGHT_GUN_IR_MIN_POINTS_FOR_SOLVE
#else
#define IR_TEST_MIN_POINTS_FOR_SOLVE 2U
#endif
#endif

#if defined(CONFIG_LIGHT_GUN_IR_COMPACT_LOG)
#define IR_TEST_ENABLE_COMPACT_LOG 1
#else
#define IR_TEST_ENABLE_COMPACT_LOG 0
#endif

#ifndef IR_TEST_LOG_EVERY_N
#ifdef CONFIG_LIGHT_GUN_IR_LOG_EVERY_N
#define IR_TEST_LOG_EVERY_N CONFIG_LIGHT_GUN_IR_LOG_EVERY_N
#else
#define IR_TEST_LOG_EVERY_N 10U
#endif
#endif

#if defined(CONFIG_LIGHT_GUN_IR_SINGLE_POINT_DEGRADE)
#define IR_TEST_ENABLE_SINGLE_POINT_DEGRADE 1
#else
#define IR_TEST_ENABLE_SINGLE_POINT_DEGRADE 0
#endif

#if defined(CONFIG_LIGHT_GUN_USB_SIM_USE_IR_LIVE_MOUSE)
#define IR_TEST_ENABLE_USB_LIVE_MOUSE_MODE 1
#else
#define IR_TEST_ENABLE_USB_LIVE_MOUSE_MODE 0
#endif

typedef struct {
    uint16_t screen_x;
    uint16_t screen_y;
    uint16_t raw_center_x;
    uint16_t raw_center_y;
    uint16_t point_spacing;
    uint8_t valid;
    uint8_t onscreen;
    uint8_t seen_count;
    uint8_t degraded;
} ir_test_runtime_solution_t;

void ir_test_overlay_entry(void);
int ir_test_get_latest_solution(ir_test_runtime_solution_t *solution);

#ifdef __cplusplus
}
#endif

#endif
