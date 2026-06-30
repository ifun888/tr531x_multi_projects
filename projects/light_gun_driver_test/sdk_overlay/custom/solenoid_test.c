#include "solenoid_test.h"

#include <stdint.h>

#include "common_def.h"
#include "gpio.h"
#include "osal_debug.h"
#include "osal_timer.h"
#include "pinctrl.h"
#include "soc_osal.h"

typedef enum {
    SOLENOID_STATE_IDLE = 0,
    SOLENOID_STATE_ON,
    SOLENOID_STATE_OFF_WAIT,
    SOLENOID_STATE_DONE,
} solenoid_state_t;

/*
 * 单个测试上下文。
 *
 * 这是一个最小化的“电磁铁闭环测试状态机”：
 * - timer: 统一的软件定时器，所有测试流程都靠它调度
 * - state: 当前处于哪一步
 * - requested_on_ms / applied_on_ms: 用于验证安全钳位
 * - target_shots / current_shots: 用于验证 burst/连发计数
 */
typedef struct {
    osal_timer timer;
    pin_t pin;
    solenoid_state_t state;
    uint32_t requested_on_ms;
    uint32_t applied_on_ms;
    uint32_t off_ms;
    uint32_t rest_ms;
    uint32_t target_shots;
    uint32_t current_shots;
    uint8_t repeating_cycle;
    uint8_t armed;
    uint8_t completion_logged;
} solenoid_test_ctx_t;

static solenoid_test_ctx_t g_solenoid_test = {
    .pin = (pin_t)SOLENOID_TEST_GPIO,
};

/* 对危险的通电时长做硬钳位，确保任何测试都不会长时间吸合线圈。 */
static uint32_t solenoid_test_clamp_on_ms(uint32_t requested_ms)
{
    if (requested_ms == 0U) {
        return 1U;
    }
    if (requested_ms > SOLENOID_TEST_MAX_ON_MS) {
        return SOLENOID_TEST_MAX_ON_MS;
    }
    return requested_ms;
}

static void solenoid_test_gpio_write(uint8_t high)
{
    (void)uapi_gpio_set_val(g_solenoid_test.pin, high ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
}

/* 强制关断输出，作为所有异常和测试结束后的安全收口。 */
static void solenoid_test_force_off(void)
{
    solenoid_test_gpio_write(0U);
    g_solenoid_test.state = SOLENOID_STATE_IDLE;
}

static void solenoid_test_timer_arm(uint32_t delay_ms)
{
    (void)osal_timer_stop(&g_solenoid_test.timer);
    (void)osal_timer_mod(&g_solenoid_test.timer, delay_ms);
}

/* 打印当前测试计划，便于串口日志和示波器波形对照。 */
static void solenoid_test_log_plan(const char *name)
{
    osal_printk("[solenoid_test] case=%s gpio=%d req_on=%u ms actual_on=%u ms off=%u ms rest=%u ms shots=%u repeat=%u\r\n",
        name,
        (int)g_solenoid_test.pin,
        (unsigned int)g_solenoid_test.requested_on_ms,
        (unsigned int)g_solenoid_test.applied_on_ms,
        (unsigned int)g_solenoid_test.off_ms,
        (unsigned int)g_solenoid_test.rest_ms,
        (unsigned int)g_solenoid_test.target_shots,
        (unsigned int)g_solenoid_test.repeating_cycle);
}

static void solenoid_test_finish_cycle(void)
{
    solenoid_test_gpio_write(0U);
    g_solenoid_test.state = SOLENOID_STATE_DONE;

    if (g_solenoid_test.repeating_cycle != 0U) {
        /*
         * 周期测试用例验证的是：
         * - 一轮测试完成后，能否稳定回到 LOW
         * - 休息一段时间后，能否重新开始下一轮
         *
         * 这里会清零计数，并进入下一轮等待。
         */
        g_solenoid_test.current_shots = 0U;
        g_solenoid_test.state = SOLENOID_STATE_IDLE;
        g_solenoid_test.completion_logged = 0U;
        solenoid_test_timer_arm(g_solenoid_test.rest_ms);
        return;
    }

    osal_printk("[solenoid_test] cycle finished, coil kept LOW.\r\n");
}

/*
 * 开始一次“有效脉冲”。
 *
 * 这个函数验证的是最核心的闭环动作：
 * 1. GPIO 拉高
 * 2. 保持固定 on 时间
 * 3. 到时后由定时器回调切回 LOW
 *
 * 如果示波器上看到的高电平宽度不对，优先看这里。
 */
static void solenoid_test_start_shot(void)
{
    g_solenoid_test.current_shots++;
    g_solenoid_test.state = SOLENOID_STATE_ON;
    solenoid_test_gpio_write(1U);
    osal_printk("[solenoid_test] shot %u/%u -> HIGH for %u ms\r\n",
        (unsigned int)g_solenoid_test.current_shots,
        (unsigned int)g_solenoid_test.target_shots,
        (unsigned int)g_solenoid_test.applied_on_ms);
    solenoid_test_timer_arm(g_solenoid_test.applied_on_ms);
}

static void solenoid_test_begin_cycle(void)
{
    if (g_solenoid_test.target_shots == 0U) {
        osal_printk("[solenoid_test] invalid target_shots=0, keep LOW.\r\n");
        solenoid_test_force_off();
        return;
    }
    solenoid_test_start_shot();
}

/*
 * 软件定时器状态机核心。
 *
 * 这个回调就是整个闭环测试的“调度中心”：
 *
 * IDLE:
 *   代表准备开始一轮测试。
 *
 * ON:
 *   代表当前线圈正在吸合。
 *   到时间后必须切回 LOW，这一步验证“通电时长控制”。
 *
 * OFF_WAIT:
 *   代表两次脉冲之间的冷却间隔。
 *   这一步验证“间隔控制”是否正确，避免脉冲粘连。
 *
 * DONE:
 *   代表单轮测试结束。
 *   非循环模式下应永久保持 LOW，验证“结束后不误触发”。
 */
static void solenoid_test_timer_cb(unsigned long arg)
{
    unused(arg);

    switch (g_solenoid_test.state) {
        case SOLENOID_STATE_IDLE:
            solenoid_test_begin_cycle();
            break;

        case SOLENOID_STATE_ON:
            solenoid_test_gpio_write(0U);
            osal_printk("[solenoid_test] shot %u/%u -> LOW cooldown %u ms\r\n",
                (unsigned int)g_solenoid_test.current_shots,
                (unsigned int)g_solenoid_test.target_shots,
                (unsigned int)g_solenoid_test.off_ms);

            if (g_solenoid_test.current_shots >= g_solenoid_test.target_shots) {
                solenoid_test_finish_cycle();
            } else {
                g_solenoid_test.state = SOLENOID_STATE_OFF_WAIT;
                solenoid_test_timer_arm(g_solenoid_test.off_ms);
            }
            break;

        case SOLENOID_STATE_OFF_WAIT:
            solenoid_test_start_shot();
            break;

        case SOLENOID_STATE_DONE:
        default:
            solenoid_test_force_off();
            break;
    }
}

static void solenoid_test_gpio_init(void)
{
    uapi_gpio_init();
    (void)uapi_pin_set_mode(g_solenoid_test.pin, HAL_PIO_FUNC_GPIO);
    (void)uapi_pin_set_pull(g_solenoid_test.pin, PIN_PULL_UP);
    (void)uapi_gpio_set_dir(g_solenoid_test.pin, GPIO_DIRECTION_OUTPUT);
    solenoid_test_force_off();
    osal_printk("[solenoid_test] gpio %d init done, default LOW with pull-up enabled.\r\n",
        (int)g_solenoid_test.pin);
}

/* 初始化统一的软件定时器，后续所有测试流程都共用这一只定时器。 */
static void solenoid_test_timer_init(void)
{
    g_solenoid_test.timer.timer = NULL;
    g_solenoid_test.timer.handler = solenoid_test_timer_cb;
    g_solenoid_test.timer.data = 0;
    g_solenoid_test.timer.interval = SOLENOID_TEST_START_DELAY_MS;
    if (osal_timer_init(&g_solenoid_test.timer) != OSAL_SUCCESS) {
        osal_printk("[solenoid_test] timer init failed.\r\n");
        return;
    }
    g_solenoid_test.armed = 1U;
}

/*
 * 根据测试宏装配测试场景。
 *
 * 这一步本质上是在构造“测试波形模板”：
 * - on 多久
 * - off 多久
 * - 一轮打几发
 * - 是否循环
 * - 每轮之间休息多久
 *
 * 每个模板都对应一个明确的验证目标。
 */
static void solenoid_test_prepare_case(void)
{
    g_solenoid_test.requested_on_ms = SOLENOID_TEST_ON_MS;
    g_solenoid_test.applied_on_ms = solenoid_test_clamp_on_ms(g_solenoid_test.requested_on_ms);
    g_solenoid_test.off_ms = SOLENOID_TEST_OFF_MS;
    g_solenoid_test.rest_ms = SOLENOID_TEST_CYCLE_REST_MS;
    g_solenoid_test.current_shots = 0U;
    g_solenoid_test.repeating_cycle = 0U;
    g_solenoid_test.target_shots = 1U;
    g_solenoid_test.state = SOLENOID_STATE_IDLE;
    g_solenoid_test.completion_logged = 0U;

#if SOLENOID_TEST_CASE == SOLENOID_TEST_CASE_SINGLE_ONCE
    /* 验证最基本的单脉冲输出，适合首次上板确认电平方向和线圈动作。 */
    g_solenoid_test.target_shots = 1U;
    g_solenoid_test.repeating_cycle = 0U;
    solenoid_test_log_plan("single_once");
#elif SOLENOID_TEST_CASE == SOLENOID_TEST_CASE_SINGLE_PERIODIC
    /* 验证长时间周期单发，重点看定时器稳定性和每轮收尾是否干净。 */
    g_solenoid_test.target_shots = 1U;
    g_solenoid_test.repeating_cycle = 1U;
    g_solenoid_test.rest_ms = SOLENOID_TEST_SINGLE_PERIOD_MS;
    solenoid_test_log_plan("single_periodic");
#elif SOLENOID_TEST_CASE == SOLENOID_TEST_CASE_AUTOFIRE_BOUNDED
    /* 验证有限连发，一轮固定多发，之后强制休息，避免线圈持续发热。 */
    g_solenoid_test.target_shots = SOLENOID_TEST_AUTOFIRE_SHOTS;
    g_solenoid_test.repeating_cycle = 1U;
    solenoid_test_log_plan("autofire_bounded");
#elif SOLENOID_TEST_CASE == SOLENOID_TEST_CASE_TRIPLE_BURST
    /* 验证三连发，重点检查 burst 计数是否准确，以及脉冲节奏是否均匀。 */
    g_solenoid_test.target_shots = SOLENOID_TEST_BURST_SHOTS;
    g_solenoid_test.repeating_cycle = 1U;
    solenoid_test_log_plan("triple_burst");
#elif SOLENOID_TEST_CASE == SOLENOID_TEST_CASE_SAFETY_CLAMP
    /* 验证安全保护：即使请求危险的长通电时间，也必须被钳位到安全值。 */
    g_solenoid_test.requested_on_ms = SOLENOID_TEST_MAX_ON_MS + 100U;
    g_solenoid_test.applied_on_ms = solenoid_test_clamp_on_ms(g_solenoid_test.requested_on_ms);
    g_solenoid_test.target_shots = 1U;
    g_solenoid_test.repeating_cycle = 1U;
    g_solenoid_test.rest_ms = SOLENOID_TEST_SINGLE_PERIOD_MS;
    solenoid_test_log_plan("safety_clamp");
#else
#error "Unsupported SOLENOID_TEST_CASE"
#endif

    if (g_solenoid_test.requested_on_ms != g_solenoid_test.applied_on_ms) {
        osal_printk("[solenoid_test] safety clamp active: requested %u ms, forced to %u ms.\r\n",
            (unsigned int)g_solenoid_test.requested_on_ms,
            (unsigned int)g_solenoid_test.applied_on_ms);
    }
}

/*
 * 主测试任务。
 *
 * 这里做三件事：
 * 1. 初始化 GPIO，确保默认 LOW
 * 2. 初始化软件定时器
 * 3. 选择测试模板并启动
 *
 * 启动前特意加了延迟，方便你上电后接示波器或逻辑分析仪。
 */
static int solenoid_test_task(void *arg)
{
    unused(arg);

    solenoid_test_gpio_init();
    solenoid_test_timer_init();
    solenoid_test_prepare_case();

    if (g_solenoid_test.armed == 0U) {
        osal_printk("[solenoid_test] startup aborted because timer is unavailable.\r\n");
        return -1;
    }

    osal_printk("[solenoid_test] start delayed by %u ms to leave time for probe hookup.\r\n",
        (unsigned int)SOLENOID_TEST_START_DELAY_MS);
    solenoid_test_timer_arm(SOLENOID_TEST_START_DELAY_MS);

    while (1) {
        osal_msleep(1000);
        if ((g_solenoid_test.state == SOLENOID_STATE_DONE) &&
            (g_solenoid_test.repeating_cycle == 0U) &&
            (g_solenoid_test.completion_logged == 0U)) {
            /* 单次测试只打印一次完成日志，避免串口刷屏影响观察。 */
            osal_printk("[solenoid_test] one-shot case completed, output remains LOW.\r\n");
            g_solenoid_test.completion_logged = 1U;
        }
    }

    return 0;
}

/* overlay 入口：创建独立测试任务，不阻塞系统其他初始化流程。 */
void solenoid_test_overlay_entry(void)
{
    osal_task *task = osal_kthread_create(solenoid_test_task, NULL, "SolenoidTest", 0x800);
    if (task == NULL) {
        osal_printk("[solenoid_test] task create failed.\r\n");
        return;
    }
    (void)osal_kthread_set_priority(task, 26);
}
