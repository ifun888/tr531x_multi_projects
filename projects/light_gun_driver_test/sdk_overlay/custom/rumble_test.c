#include "rumble_test.h"

#include <stdbool.h>
#include <stdint.h>

#include "common_def.h"
#include "gpio.h"
#include "osal_debug.h"
#include "osal_timer.h"
#include "pinctrl.h"
#include "pwm.h"
#include "soc_osal.h"

typedef enum {
    RUMBLE_STATE_IDLE = 0,
    RUMBLE_STATE_ACTIVE,
    RUMBLE_STATE_GAP_WAIT,
    RUMBLE_STATE_DONE,
} rumble_state_t;

/*
 * 振动马达闭环测试上下文。
 *
 * 这是一个最小化 PWM 振动状态机：
 * - level_percent: 当前目标占空比，验证“强度映射”
 * - active_ms / gap_ms: 验证“振动持续时间”和“脉冲间隔”
 * - target_pulses / current_pulses: 验证有限连振、周期振动、sweep 步进
 */
typedef struct {
    osal_timer timer;
    uint8_t channel;
    uint8_t group_id;
    pin_t pin;
    uint32_t requested_level_percent;
    uint32_t applied_level_percent;
    uint32_t requested_active_ms;
    uint32_t applied_active_ms;
    uint32_t gap_ms;
    uint32_t rest_ms;
    uint32_t target_pulses;
    uint32_t current_pulses;
    uint8_t repeating_cycle;
    uint8_t armed;
    uint8_t completion_logged;
    uint8_t pwm_started;
    rumble_state_t state;
} rumble_test_ctx_t;

static rumble_test_ctx_t g_rumble_test = {
    .channel = (uint8_t)RUMBLE_TEST_PWM_CHANNEL,
    .group_id = (uint8_t)RUMBLE_TEST_PWM_GROUP_ID,
    .pin = (pin_t)RUMBLE_TEST_PWM_PIN,
};

#define RUMBLE_TEST_PWM_MODE_MIN ((pin_mode_t)HAL_PIO_PWM0)
#define RUMBLE_TEST_PWM_MODE_MAX ((pin_mode_t)HAL_PIO_PWM11)

static uint32_t rumble_test_clamp_level_percent(uint32_t requested_percent)
{
    if (requested_percent > RUMBLE_TEST_MAX_LEVEL_PERCENT) {
        return RUMBLE_TEST_MAX_LEVEL_PERCENT;
    }
    return requested_percent;
}

static uint32_t rumble_test_clamp_active_ms(uint32_t requested_ms)
{
    if (requested_ms == 0U) {
        return 1U;
    }
    if (requested_ms > RUMBLE_TEST_MAX_ACTIVE_MS) {
        return RUMBLE_TEST_MAX_ACTIVE_MS;
    }
    return requested_ms;
}

/*
 * 空闲态强制拉成 GPIO 低电平。
 *
 * 这样示波器上会看到一条明确的低线，
 * 不会把“PWM 已启动但占空比为 0%”和“真正关断”混在一起。
 */
static void rumble_test_gpio_force_low(void)
{
    uapi_gpio_init();
    (void)uapi_pin_set_mode(g_rumble_test.pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_pull(g_rumble_test.pin, PIN_PULL_DOWN);
    (void)uapi_gpio_set_dir(g_rumble_test.pin, GPIO_DIRECTION_OUTPUT);
    (void)uapi_gpio_set_val(g_rumble_test.pin, GPIO_LEVEL_LOW);
}

/*
 * 清理残留 PWM pinmux。
 *
 * TR531x 的 pinmux 不会帮我们做“同功能排他解绑”：
 * 只要两个 IO 都被切到同一个 PWM 功能，它们就可能一起输出同一路 PWM。
 *
 * 这里在测试启动时把所有仍处于 PWM 复用模式的 GPIO 都切回普通 GPIO 低电平，
 * 避免上一次试验留下的旧引脚继续出波形。
 *
 * 注意：
 * - 这是测试工程，默认认为当前测试独占 PWM 外设
 * - 如果后续工程里有别的 PWM 业务共存，就不能这样全清
 */
static void rumble_test_cleanup_stale_pwm_pins(void)
{
    pin_t pin;

    uapi_gpio_init();
    for (pin = S_MGPIO0; pin <= S_MGPIO31; pin++) {
        pin_mode_t current_mode = uapi_pin_get_mode(pin);
        if ((current_mode < RUMBLE_TEST_PWM_MODE_MIN) || (current_mode > RUMBLE_TEST_PWM_MODE_MAX)) {
            continue;
        }

        (void)uapi_pin_set_mode(pin, HAL_PIO_FUNC_GPIO);
        (void)uapi_pin_set_pull(pin, PIN_PULL_DOWN);
        (void)uapi_gpio_set_dir(pin, GPIO_DIRECTION_OUTPUT);
        (void)uapi_gpio_set_val(pin, GPIO_LEVEL_LOW);

        osal_printk("[rumble_test] cleanup stale pwm mux on gpio=%d mode=%u -> GPIO LOW.\r\n",
            (int)pin,
            (unsigned int)current_mode);
    }
}

static int rumble_test_apply_level(uint32_t level_percent)
{
    pwm_config_t cfg;
    uint32_t clamped_level = rumble_test_clamp_level_percent(level_percent);
    errcode_t ret;

    cfg.low_time = 100U - clamped_level;
    cfg.high_time = clamped_level;
    cfg.offset_time = 0;
    cfg.cycles = 0xFF;
    cfg.repeat = true;

    ret = uapi_pin_set_mode(g_rumble_test.pin, (pin_mode_t)RUMBLE_TEST_PWM_PIN_MODE);
    if (ret != ERRCODE_SUCC) {
        osal_printk("[rumble_test] uapi_pin_set_mode failed before update, pin=%d mode=%u ret=%d.\r\n",
            (int)g_rumble_test.pin,
            (unsigned int)RUMBLE_TEST_PWM_PIN_MODE,
            (int)ret);
        return -1;
    }

    ret = uapi_pwm_update_cfg(g_rumble_test.channel, &cfg);
    if (ret != ERRCODE_SUCC) {
        osal_printk("[rumble_test] uapi_pwm_update_cfg failed, ch=%u ret=%d low=%u high=%u.\r\n",
            (unsigned int)g_rumble_test.channel,
            (int)ret,
            (unsigned int)cfg.low_time,
            (unsigned int)cfg.high_time);
        return -1;
    }
    return 0;
}

/* 强制关闭振动输出，作为所有测试结束和异常时的安全收口。 */
static void rumble_test_force_off(void)
{
    rumble_test_gpio_force_low();
    g_rumble_test.state = RUMBLE_STATE_IDLE;
}

static void rumble_test_timer_arm(uint32_t delay_ms)
{
    (void)osal_timer_stop(&g_rumble_test.timer);
    (void)osal_timer_mod(&g_rumble_test.timer, delay_ms);
}

/* 打印当前测试计划，便于串口日志和示波器/听感对照。 */
static void rumble_test_log_plan(const char *name)
{
    osal_printk("[rumble_test] case=%s ch=%u group=%u pin=%d req_level=%u%% actual_level=%u%% req_active=%u ms actual_active=%u ms gap=%u ms rest=%u ms pulses=%u repeat=%u\r\n",
        name,
        (unsigned int)g_rumble_test.channel,
        (unsigned int)g_rumble_test.group_id,
        (int)g_rumble_test.pin,
        (unsigned int)g_rumble_test.requested_level_percent,
        (unsigned int)g_rumble_test.applied_level_percent,
        (unsigned int)g_rumble_test.requested_active_ms,
        (unsigned int)g_rumble_test.applied_active_ms,
        (unsigned int)g_rumble_test.gap_ms,
        (unsigned int)g_rumble_test.rest_ms,
        (unsigned int)g_rumble_test.target_pulses,
        (unsigned int)g_rumble_test.repeating_cycle);
}

static uint32_t rumble_test_compute_level_for_step(uint32_t step_index)
{
#if RUMBLE_TEST_CASE == RUMBLE_TEST_CASE_DUTY_SWEEP
    static const uint8_t level_table[] = {25U, 50U, 75U, 100U};
    uint32_t capped_index = step_index;
    if (capped_index >= (sizeof(level_table) / sizeof(level_table[0]))) {
        capped_index = (sizeof(level_table) / sizeof(level_table[0])) - 1U;
    }
    return level_table[capped_index];
#else
    unused(step_index);
    return g_rumble_test.applied_level_percent;
#endif
}

static void rumble_test_finish_cycle(void)
{
    rumble_test_gpio_force_low();
    g_rumble_test.state = RUMBLE_STATE_DONE;

    if (g_rumble_test.repeating_cycle != 0U) {
        /*
         * 周期测试用例验证的是：
         * - 一轮完成后是否能稳定回到 GPIO 低电平
         * - 休息一段时间后能否重新开始
         */
        g_rumble_test.current_pulses = 0U;
        g_rumble_test.state = RUMBLE_STATE_IDLE;
        g_rumble_test.completion_logged = 0U;
        rumble_test_timer_arm(g_rumble_test.rest_ms);
        return;
    }

    osal_printk("[rumble_test] cycle finished, output forced LOW.\r\n");
}

/*
 * 开始一次有效振动脉冲。
 *
 * 这个函数验证的是：
 * 1. 占空比是否能切换到目标强度
 * 2. 目标强度持续时间是否正确
 * 3. 到时后能否切回 GPIO 低电平
 */
static void rumble_test_start_pulse(void)
{
    uint32_t pulse_index = g_rumble_test.current_pulses;
    uint32_t level_percent = rumble_test_compute_level_for_step(pulse_index);

    g_rumble_test.current_pulses++;
    g_rumble_test.state = RUMBLE_STATE_ACTIVE;
    (void)rumble_test_apply_level(level_percent);
    osal_printk("[rumble_test] pulse %u/%u -> duty %u%% for %u ms\r\n",
        (unsigned int)g_rumble_test.current_pulses,
        (unsigned int)g_rumble_test.target_pulses,
        (unsigned int)level_percent,
        (unsigned int)g_rumble_test.applied_active_ms);
    rumble_test_timer_arm(g_rumble_test.applied_active_ms);
}

static void rumble_test_begin_cycle(void)
{
    if (g_rumble_test.target_pulses == 0U) {
        osal_printk("[rumble_test] invalid target_pulses=0, keep GPIO LOW.\r\n");
        rumble_test_force_off();
        return;
    }
    rumble_test_start_pulse();
}

/*
 * 软件定时器状态机核心。
 *
 * IDLE:
 *   准备开始一轮振动测试。
 *
 * ACTIVE:
 *   当前正在震动，验证“占空比输出 + 持续时间”。
 *
 * GAP_WAIT:
 *   当前处于两次振动之间的停顿，验证“间隔控制”。
 *
 * DONE:
 *   当前测试完成，验证“结束后保持 GPIO 低电平，不再误触发”。
 */
static void rumble_test_timer_cb(unsigned long arg)
{
    unused(arg);

    switch (g_rumble_test.state) {
        case RUMBLE_STATE_IDLE:
            rumble_test_begin_cycle();
            break;

        case RUMBLE_STATE_ACTIVE:
            rumble_test_gpio_force_low();
            osal_printk("[rumble_test] pulse %u/%u -> GPIO LOW gap %u ms\r\n",
                (unsigned int)g_rumble_test.current_pulses,
                (unsigned int)g_rumble_test.target_pulses,
                (unsigned int)g_rumble_test.gap_ms);

            if (g_rumble_test.current_pulses >= g_rumble_test.target_pulses) {
                rumble_test_finish_cycle();
            } else {
                g_rumble_test.state = RUMBLE_STATE_GAP_WAIT;
                rumble_test_timer_arm(g_rumble_test.gap_ms);
            }
            break;

        case RUMBLE_STATE_GAP_WAIT:
            rumble_test_start_pulse();
            break;

        case RUMBLE_STATE_DONE:
        default:
            rumble_test_force_off();
            break;
    }
}

static void rumble_test_pwm_init(void)
{
    pwm_config_t cfg;
    errcode_t ret;

    rumble_test_cleanup_stale_pwm_pins();

    ret = uapi_pin_set_mode(g_rumble_test.pin, (pin_mode_t)RUMBLE_TEST_PWM_PIN_MODE);
    if (ret != ERRCODE_SUCC) {
        osal_printk("[rumble_test] uapi_pin_set_mode failed, pin=%d mode=%u ret=%d.\r\n",
            (int)g_rumble_test.pin,
            (unsigned int)RUMBLE_TEST_PWM_PIN_MODE,
            (int)ret);
        return;
    }

    /*
     * 先用一个明确的非 0 duty 把 PWM 底座启动起来。
     * 真正的“关闭输出”不靠 0% duty，而是切回 GPIO 低电平。
     */
    cfg.low_time = 50U;
    cfg.high_time = 50U;
    cfg.offset_time = 0U;
    cfg.cycles = 0xFFU;
    cfg.repeat = true;

    ret = uapi_pwm_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[rumble_test] uapi_pwm_init failed, ret=%d.\r\n", (int)ret);
        return;
    }

    ret = uapi_pwm_open(g_rumble_test.channel, &cfg);
    if (ret != ERRCODE_SUCC) {
        osal_printk("[rumble_test] uapi_pwm_open failed, ch=%u ret=%d.\r\n",
            (unsigned int)g_rumble_test.channel, (int)ret);
        return;
    }

#if defined(CONFIG_PWM_USING_V151)
    {
        uint8_t channel_id = g_rumble_test.channel;
        ret = uapi_pwm_set_group(g_rumble_test.group_id, &channel_id, 1);
        if (ret != ERRCODE_SUCC) {
            osal_printk("[rumble_test] uapi_pwm_set_group failed, group=%u ch=%u ret=%d.\r\n",
                (unsigned int)g_rumble_test.group_id,
                (unsigned int)g_rumble_test.channel,
                (int)ret);
            return;
        }

        ret = uapi_pwm_start_group(g_rumble_test.group_id);
        if (ret != ERRCODE_SUCC) {
            osal_printk("[rumble_test] uapi_pwm_start_group failed, group=%u ret=%d.\r\n",
                (unsigned int)g_rumble_test.group_id,
                (int)ret);
            return;
        }
    }
#else
    ret = uapi_pwm_start(g_rumble_test.channel);
    if (ret != ERRCODE_SUCC) {
        osal_printk("[rumble_test] uapi_pwm_start failed, ch=%u ret=%d.\r\n",
            (unsigned int)g_rumble_test.channel, (int)ret);
        return;
    }
#endif

    g_rumble_test.pwm_started = 1U;
    rumble_test_gpio_force_low();
    osal_printk("[rumble_test] pwm init done, channel=%u group=%u pin=%d mode=%u, idle forced LOW.\r\n",
        (unsigned int)g_rumble_test.channel,
        (unsigned int)g_rumble_test.group_id,
        (int)g_rumble_test.pin,
        (unsigned int)RUMBLE_TEST_PWM_PIN_MODE);
}

/* 初始化统一的软件定时器，所有振动测试流程都复用它。 */
static void rumble_test_timer_init(void)
{
    g_rumble_test.timer.timer = NULL;
    g_rumble_test.timer.handler = rumble_test_timer_cb;
    g_rumble_test.timer.data = 0;
    g_rumble_test.timer.interval = RUMBLE_TEST_START_DELAY_MS;
    if (osal_timer_init(&g_rumble_test.timer) != OSAL_SUCCESS) {
        osal_printk("[rumble_test] timer init failed.\r\n");
        return;
    }
    g_rumble_test.armed = 1U;
}

/*
 * 根据测试宏装配测试场景。
 *
 * 每个模板都在验证不同的能力：
 * - 基础单脉冲
 * - 周期性振动
 * - 有限多脉冲连振
 * - 强度映射
 * - 安全钳位
 */
static void rumble_test_prepare_case(void)
{
    g_rumble_test.requested_level_percent = RUMBLE_TEST_LEVEL_PERCENT;
    g_rumble_test.applied_level_percent = rumble_test_clamp_level_percent(g_rumble_test.requested_level_percent);
    g_rumble_test.requested_active_ms = RUMBLE_TEST_ACTIVE_MS;
    g_rumble_test.applied_active_ms = rumble_test_clamp_active_ms(g_rumble_test.requested_active_ms);
    g_rumble_test.gap_ms = RUMBLE_TEST_GAP_MS;
    g_rumble_test.rest_ms = RUMBLE_TEST_CYCLE_REST_MS;
    g_rumble_test.current_pulses = 0U;
    g_rumble_test.repeating_cycle = 0U;
    g_rumble_test.target_pulses = 1U;
    g_rumble_test.completion_logged = 0U;
    g_rumble_test.state = RUMBLE_STATE_IDLE;

#if RUMBLE_TEST_CASE == RUMBLE_TEST_CASE_CONTINUOUS_DIAG
    /*
     * 诊断模式只验证最底层链路：
     * GPIO1 -> pinmux -> PWM1 -> 外部驱动输入。
     *
     * 这个模式不走脉冲状态机，方便先确认“有没有稳定 PWM 波形”。
     */
    g_rumble_test.target_pulses = 1U;
    g_rumble_test.repeating_cycle = 0U;
    rumble_test_log_plan("continuous_diag");
#elif RUMBLE_TEST_CASE == RUMBLE_TEST_CASE_SINGLE_PULSE_ONCE
    /* 验证最基础的单次振动脉冲。 */
    g_rumble_test.target_pulses = 1U;
    g_rumble_test.repeating_cycle = 0U;
    rumble_test_log_plan("single_pulse_once");
#elif RUMBLE_TEST_CASE == RUMBLE_TEST_CASE_PERIODIC_PULSE
    /* 验证长时间周期短振动，重点看定时器稳定性和每轮收尾是否正确。 */
    g_rumble_test.target_pulses = 1U;
    g_rumble_test.repeating_cycle = 1U;
    g_rumble_test.rest_ms = RUMBLE_TEST_PERIOD_MS;
    rumble_test_log_plan("periodic_pulse");
#elif RUMBLE_TEST_CASE == RUMBLE_TEST_CASE_BOUNDED_AUTOFIRE
    /* 验证有限多脉冲连续振动，一轮固定多次，之后强制休息。 */
    g_rumble_test.target_pulses = RUMBLE_TEST_AUTOFIRE_PULSES;
    g_rumble_test.repeating_cycle = 1U;
    rumble_test_log_plan("bounded_autofire");
#elif RUMBLE_TEST_CASE == RUMBLE_TEST_CASE_DUTY_SWEEP
    /* 验证占空比和体感强度的映射，默认从 25% -> 50% -> 75% -> 100%。 */
    g_rumble_test.target_pulses = RUMBLE_TEST_SWEEP_STEPS;
    g_rumble_test.repeating_cycle = 1U;
    rumble_test_log_plan("duty_sweep");
#elif RUMBLE_TEST_CASE == RUMBLE_TEST_CASE_SAFETY_CLAMP
    /* 验证安全钳位，故意请求超范围强度和过长持续时间。 */
    g_rumble_test.requested_level_percent = RUMBLE_TEST_MAX_LEVEL_PERCENT + 50U;
    g_rumble_test.applied_level_percent = rumble_test_clamp_level_percent(g_rumble_test.requested_level_percent);
    g_rumble_test.requested_active_ms = RUMBLE_TEST_MAX_ACTIVE_MS + 500U;
    g_rumble_test.applied_active_ms = rumble_test_clamp_active_ms(g_rumble_test.requested_active_ms);
    g_rumble_test.target_pulses = 1U;
    g_rumble_test.repeating_cycle = 1U;
    g_rumble_test.rest_ms = RUMBLE_TEST_PERIOD_MS;
    rumble_test_log_plan("safety_clamp");
#else
#error "Unsupported RUMBLE_TEST_CASE"
#endif

    if (g_rumble_test.requested_level_percent != g_rumble_test.applied_level_percent) {
        osal_printk("[rumble_test] safety clamp active: requested level %u%%, forced to %u%%.\r\n",
            (unsigned int)g_rumble_test.requested_level_percent,
            (unsigned int)g_rumble_test.applied_level_percent);
    }
    if (g_rumble_test.requested_active_ms != g_rumble_test.applied_active_ms) {
        osal_printk("[rumble_test] safety clamp active: requested active %u ms, forced to %u ms.\r\n",
            (unsigned int)g_rumble_test.requested_active_ms,
            (unsigned int)g_rumble_test.applied_active_ms);
    }
}

/*
 * 主测试任务。
 *
 * 这里做三件事：
 * 1. 初始化 PWM 输出底座，空闲态回到 GPIO 低电平
 * 2. 初始化软件定时器
 * 3. 选择测试模板并启动
 *
 * 启动前同样保留延迟，方便你连示波器、听电机动作或摸驱动板反应。
 */
static int rumble_test_task(void *arg)
{
    unused(arg);

    rumble_test_pwm_init();
    rumble_test_timer_init();
    rumble_test_prepare_case();

    if ((g_rumble_test.armed == 0U) || (g_rumble_test.pwm_started == 0U)) {
        osal_printk("[rumble_test] startup aborted because timer or pwm is unavailable.\r\n");
        return -1;
    }

#if RUMBLE_TEST_CASE == RUMBLE_TEST_CASE_CONTINUOUS_DIAG
    osal_printk("[rumble_test] continuous diagnostic mode: expect stable PWM on GPIO%d, mode=%u, duty=%u%%.\r\n",
        (int)g_rumble_test.pin,
        (unsigned int)RUMBLE_TEST_PWM_PIN_MODE,
        (unsigned int)g_rumble_test.applied_level_percent);
    if (rumble_test_apply_level(g_rumble_test.applied_level_percent) != 0) {
        osal_printk("[rumble_test] failed to enable continuous diagnostic output.\r\n");
        return -1;
    }
    while (1) {
        osal_msleep(1000);
        osal_printk("[rumble_test] diagnostic alive: probe GPIO%d for continuous PWM.\r\n",
            (int)g_rumble_test.pin);
    }
#endif

    osal_printk("[rumble_test] start delayed by %u ms to leave time for probe hookup.\r\n",
        (unsigned int)RUMBLE_TEST_START_DELAY_MS);
    rumble_test_timer_arm(RUMBLE_TEST_START_DELAY_MS);

    while (1) {
        osal_msleep(1000);
        if ((g_rumble_test.state == RUMBLE_STATE_DONE) &&
            (g_rumble_test.repeating_cycle == 0U) &&
            (g_rumble_test.completion_logged == 0U)) {
            /* 单次测试只打印一次完成日志，避免串口刷屏影响观察。 */
            osal_printk("[rumble_test] one-shot case completed, output remains GPIO LOW.\r\n");
            g_rumble_test.completion_logged = 1U;
        }
    }

    return 0;
}

/* overlay 入口：创建独立测试任务，不阻塞系统其他初始化流程。 */
void rumble_test_overlay_entry(void)
{
    osal_task *task = osal_kthread_create(rumble_test_task, NULL, "RumbleTest", 0x800);
    if (task == NULL) {
        osal_printk("[rumble_test] task create failed.\r\n");
        return;
    }
    (void)osal_kthread_set_priority(task, 26);
}
