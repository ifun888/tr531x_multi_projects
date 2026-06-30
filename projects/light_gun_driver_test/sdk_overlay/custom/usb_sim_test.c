#include "usb_sim_test.h"

#include <stdbool.h>
#include <stdint.h>

#include "common_def.h"
#include "gpio.h"
#include "ir_test.h"
#include "implementation/usb_init.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "pinctrl.h"
#include "soc_osal.h"
#include "gadget/f_hid.h"
#if defined(CONFIG_LIGHT_GUN_USB_DEVICE_MODE_ACM_HID) && defined(CONFIG_LIGHT_GUN_USB_CDC_ECHO_TEST)
#include "console.h"
#include "gadget/usbd_acm.h"
#endif

#define USB_SIM_TASK_STACK_SIZE                0x1000
#define USB_SIM_TASK_PRIO                      25
#define USB_SIM_HID_INIT_DELAY_MS              500U
#define USB_SIM_CDC_TASK_STACK_SIZE            0x1000
#define USB_SIM_CDC_TASK_PRIO                  24
#define USB_SIM_KEYBOARD_REPORT_LEN            9U
#define USB_SIM_MOUSE_REPORT_LEN               5U
#define USB_SIM_GAMEPAD_REPORT_LEN             12U
#define USB_SIM_MAX_KEYS                       6U
#define USB_SIM_INVALID_PIN                    255U
#define USB_SIM_LED_COUNT                      3U
#define USB_SIM_LINK_FAIL_THRESHOLD            3U
#define USB_SIM_CDC_ECHO_BUF_LEN               64U
#define USB_SIM_GAMEPAD_INVALID_BUTTON         0xFFU
#define USB_SIM_GAMEPAD_HAT_NEUTRAL            8U
#define USB_SIM_FAKE_IR_AXIS_MAX_X             640U
#define USB_SIM_FAKE_IR_AXIS_MAX_Y             360U

#define input(size)             (0x80 | (size))
#define output(size)            (0x90 | (size))
#define collection(size)        (0xa0 | (size))
#define end_collection(size)    (0xc0 | (size))
#define usage_page(size)        (0x04 | (size))
#define logical_minimum(size)   (0x14 | (size))
#define logical_maximum(size)   (0x24 | (size))
#define report_size(size)       (0x74 | (size))
#define report_id(size)         (0x84 | (size))
#define report_count(size)      (0x94 | (size))
#define usage(size)             (0x08 | (size))
#define usage_minimum(size)     (0x18 | (size))
#define usage_maximum(size)     (0x28 | (size))

#define USB_SIM_HID_REPORT_ID_KEYBOARD 0x01
#define USB_SIM_HID_REPORT_ID_MOUSE    0x04
#define USB_SIM_HID_REPORT_ID_GAMEPAD  0x05
#define USB_SIM_HID_PROTOCOL_COMPOSITE 0

#if defined(CONFIG_LIGHT_GUN_USB_DEVICE_MODE_ACM_HID)
#define USB_SIM_DEVICE_TYPE DEV_SER_HID
#define USB_SIM_DEVICE_MODE_NAME "CDC ACM + HID"
#else
#define USB_SIM_DEVICE_TYPE DEV_HID
#define USB_SIM_DEVICE_MODE_NAME "HID"
#endif

#define HID_KEY_A          0x04
#define HID_KEY_B          0x05
#define HID_KEY_ENTER      0x28
#define HID_KEY_ESC        0x29
#define HID_KEY_HOME       0x4A
#define HID_KEY_RIGHT      0x4F
#define HID_KEY_LEFT       0x50
#define HID_KEY_DOWN       0x51
#define HID_KEY_UP         0x52

typedef enum {
    USB_SIM_GAMEPAD_BTN_PRIMARY = 0,
    USB_SIM_GAMEPAD_BTN_SECONDARY,
    USB_SIM_GAMEPAD_BTN_TERTIARY,
    USB_SIM_GAMEPAD_BTN_QUATERNARY,
    USB_SIM_GAMEPAD_BTN_START = 11,
    USB_SIM_GAMEPAD_BTN_SELECT = 10,
    USB_SIM_GAMEPAD_BTN_HOME = 12,
} usb_sim_gamepad_button_t;

typedef enum {
    USB_SIM_DPAD_NONE = 0,
    USB_SIM_DPAD_UP,
    USB_SIM_DPAD_DOWN,
    USB_SIM_DPAD_LEFT,
    USB_SIM_DPAD_RIGHT,
} usb_sim_dpad_dir_t;

typedef union {
    struct {
        uint8_t left_key : 1;
        uint8_t right_key : 1;
        uint8_t mid_key : 1;
        uint8_t reserved : 5;
    } b;
    uint8_t d8;
} usb_sim_mouse_key_t;

typedef struct {
    uint8_t kind;
    usb_sim_mouse_key_t key;
    int8_t x;
    int8_t y;
    int8_t wheel;
} usb_sim_mouse_report_t;

typedef struct {
    uint8_t kind;
    uint8_t special_key;
    uint8_t reserve;
    uint8_t key[USB_SIM_MAX_KEYS];
} usb_sim_keyboard_report_t;

typedef struct __attribute__((packed)) {
    uint8_t kind;
    int16_t x;
    int16_t y;
    int16_t rx;
    int16_t ry;
    uint8_t hat;
    uint16_t buttons;
} usb_sim_gamepad_report_t;

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t onscreen;
} usb_sim_ir_point_t;

typedef enum {
    USB_BTN_ROLE_TRIGGER = 0,
    USB_BTN_ROLE_MOUSE_RIGHT,
    USB_BTN_ROLE_MOUSE_MIDDLE,
    USB_BTN_ROLE_KEYBOARD,
    USB_BTN_ROLE_RESERVED,
} usb_sim_btn_role_t;

typedef struct {
    const char *name;
    pin_t pin;
    usb_sim_btn_role_t role;
    uint8_t key_code;
    uint8_t gamepad_button;
    uint8_t dpad_dir;
    uint8_t enabled;
    uint8_t stable_pressed;
    uint8_t raw_pressed;
    uint32_t debounce_ms;
} usb_sim_button_t;

typedef struct {
    uint8_t mouse_left;
    uint8_t mouse_right;
    uint8_t mouse_middle;
    uint8_t keyboard_keys[USB_SIM_MAX_KEYS];
    uint8_t keyboard_key_count;
    uint8_t trigger_offscreen_active;
    uint8_t dpad_up;
    uint8_t dpad_down;
    uint8_t dpad_left;
    uint8_t dpad_right;
    uint8_t gamepad_hat;
    uint16_t gamepad_buttons;
    int16_t gamepad_x;
    int16_t gamepad_y;
    int16_t gamepad_rx;
    int16_t gamepad_ry;
} usb_sim_report_state_t;

typedef struct {
    uint8_t usb_ready;
    uint8_t usb_active;
    uint8_t hid_index;
    uint32_t loop_ms;
    uint32_t scripted_elapsed_ms;
    uint32_t trigger_mode_elapsed_ms;
    uint32_t script_step_index;
    uint32_t link_probe_elapsed_ms;
    uint8_t link_fail_count;
    uint32_t prev_ir_x;
    uint32_t prev_ir_y;
    uint8_t prev_ir_valid;
    uint16_t live_ir_last_x;
    uint16_t live_ir_last_y;
    uint8_t live_ir_prev_valid;
    uint8_t led_state[USB_SIM_LED_COUNT];
    uint8_t current_onscreen;
    usb_sim_report_state_t report_state;
} usb_sim_ctx_t;

static const uint8_t g_usb_sim_report_desc_hid[] = {
    usage_page(1),      0x01,
    usage(1),           0x06,
    collection(1),      0x01,
    report_id(1),       USB_SIM_HID_REPORT_ID_KEYBOARD,
    usage_page(1),      0x07,
    usage_minimum(1),   0xE0,
    usage_maximum(1),   0xE7,
    logical_minimum(1), 0x00,
    logical_maximum(1), 0x01,
    report_size(1),     0x01,
    report_count(1),    0x08,
    input(1),           0x02,
    report_count(1),    0x01,
    report_size(1),     0x08,
    input(1),           0x01,
    report_count(1),    0x05,
    report_size(1),     0x01,
    usage_page(1),      0x08,
    usage_minimum(1),   0x01,
    usage_maximum(1),   0x05,
    output(1),          0x02,
    report_count(1),    0x01,
    report_size(1),     0x03,
    output(1),          0x01,
    report_count(1),    0x06,
    report_size(1),     0x08,
    logical_minimum(1), 0x00,
    logical_maximum(1), 0x65,
    usage_page(1),      0x07,
    usage_minimum(1),   0x00,
    usage_maximum(1),   0x65,
    input(1),           0x00,
    end_collection(0),
    usage_page(1),      0x01,
    usage(1),           0x02,
    collection(1),      0x01,
    report_id(1),       USB_SIM_HID_REPORT_ID_MOUSE,
    usage(1),           0x01,
    collection(1),      0x00,
    report_count(1),    0x03,
    report_size(1),     0x01,
    usage_page(1),      0x09,
    usage_minimum(1),   0x01,
    usage_maximum(1),   0x03,
    logical_minimum(1), 0x00,
    logical_maximum(1), 0x01,
    input(1),           0x02,
    report_count(1),    0x01,
    report_size(1),     0x05,
    input(1),           0x01,
    report_count(1),    0x03,
    report_size(1),     0x08,
    usage_page(1),      0x01,
    usage(1),           0x30,
    usage(1),           0x31,
    usage(1),           0x38,
    logical_minimum(1), 0x81,
    logical_maximum(1), 0x7F,
    input(1),           0x06,
    end_collection(0),
    end_collection(0),
    usage_page(1),      0x01,
    usage(1),           0x05,
    collection(1),      0x01,
    report_id(1),       USB_SIM_HID_REPORT_ID_GAMEPAD,
    usage(1),           0x30,
    usage(1),           0x31,
    usage(1),           0x33,
    usage(1),           0x34,
    logical_minimum(2), 0x01, 0x80,
    logical_maximum(2), 0xFF, 0x7F,
    report_count(1),    0x04,
    report_size(1),     0x10,
    input(1),           0x02,
    usage(1),           0x39,
    logical_minimum(1), 0x00,
    logical_maximum(1), 0x08,
    report_count(1),    0x01,
    report_size(1),     0x08,
    input(1),           0x42,
    usage_page(1),      0x09,
    usage_minimum(1),   0x01,
    usage_maximum(1),   0x10,
    logical_minimum(1), 0x00,
    logical_maximum(1), 0x01,
    report_count(1),    0x10,
    report_size(1),     0x01,
    input(1),           0x02,
    end_collection(0),
};

static const usb_sim_ir_point_t g_usb_sim_ir_script[] = {
    { 200, 120, 1 },
    { 360, 120, 1 },
    { 360, 260, 1 },
    { 200, 260, 1 },
    { 0, 0, 0 },
    { 260, 180, 1 },
    { 420, 180, 1 },
    { 0, 0, 0 },
};

static usb_sim_button_t g_usb_sim_buttons[] = {
    { "TRIGGER", (pin_t)USB_SIM_TEST_BTN_TRIGGER_PIN, USB_BTN_ROLE_TRIGGER, 0, USB_SIM_GAMEPAD_BTN_PRIMARY, USB_SIM_DPAD_NONE, 1, 0, 0, 0 },
    { "A",       (pin_t)USB_SIM_TEST_BTN_A_PIN,       USB_BTN_ROLE_MOUSE_RIGHT, 0, USB_SIM_GAMEPAD_BTN_SECONDARY, USB_SIM_DPAD_NONE, 1, 0, 0, 0 },
    { "B",       (pin_t)USB_SIM_TEST_BTN_B_PIN,       USB_BTN_ROLE_MOUSE_MIDDLE, 0, USB_SIM_GAMEPAD_BTN_TERTIARY, USB_SIM_DPAD_NONE, 1, 0, 0, 0 },
    { "START",   (pin_t)USB_SIM_TEST_BTN_START_PIN,   USB_BTN_ROLE_KEYBOARD, HID_KEY_ENTER, USB_SIM_GAMEPAD_BTN_START, USB_SIM_DPAD_NONE, 1, 0, 0, 0 },
    { "SELECT",  (pin_t)USB_SIM_TEST_BTN_SELECT_PIN,  USB_BTN_ROLE_KEYBOARD, HID_KEY_ESC, USB_SIM_GAMEPAD_BTN_SELECT, USB_SIM_DPAD_NONE, 1, 0, 0, 0 },
    { "HOME",    (pin_t)USB_SIM_TEST_BTN_HOME_PIN,    USB_BTN_ROLE_KEYBOARD, HID_KEY_HOME, USB_SIM_GAMEPAD_BTN_HOME, USB_SIM_DPAD_NONE, 1, 0, 0, 0 },
    { "GAMEPAD", (pin_t)USB_SIM_TEST_BTN_GAMEPAD_PIN, USB_BTN_ROLE_RESERVED, 0, USB_SIM_GAMEPAD_BTN_QUATERNARY, USB_SIM_DPAD_NONE, 1, 0, 0, 0 },
    { "KEY_UP",  (pin_t)USB_SIM_TEST_BTN_UP_PIN,      USB_BTN_ROLE_KEYBOARD, HID_KEY_UP, USB_SIM_GAMEPAD_INVALID_BUTTON, USB_SIM_DPAD_UP, 1, 0, 0, 0 },
    { "KEY_DOWN",(pin_t)USB_SIM_TEST_BTN_DOWN_PIN,    USB_BTN_ROLE_KEYBOARD, HID_KEY_DOWN, USB_SIM_GAMEPAD_INVALID_BUTTON, USB_SIM_DPAD_DOWN, 1, 0, 0, 0 },
    { "KEY_LEFT",(pin_t)USB_SIM_TEST_BTN_LEFT_PIN,    USB_BTN_ROLE_KEYBOARD, HID_KEY_LEFT, USB_SIM_GAMEPAD_INVALID_BUTTON, USB_SIM_DPAD_LEFT, 1, 0, 0, 0 },
    { "KEY_RIGHT",(pin_t)USB_SIM_TEST_BTN_RIGHT_PIN,  USB_BTN_ROLE_KEYBOARD, HID_KEY_RIGHT, USB_SIM_GAMEPAD_INVALID_BUTTON, USB_SIM_DPAD_RIGHT, 1, 0, 0, 0 },
    { "KEY_MIDDLE",(pin_t)USB_SIM_TEST_BTN_MIDDLE_PIN,USB_BTN_ROLE_KEYBOARD, HID_KEY_B, USB_SIM_GAMEPAD_INVALID_BUTTON, USB_SIM_DPAD_NONE, 1, 0, 0, 0 },
};

static usb_sim_ctx_t g_usb_sim_ctx = {
    .current_onscreen = 1,
};

#if defined(CONFIG_LIGHT_GUN_USB_DEVICE_MODE_ACM_HID) && defined(CONFIG_LIGHT_GUN_USB_CDC_ECHO_TEST)
static char g_usb_sim_cdc_echo_buf[USB_SIM_CDC_ECHO_BUF_LEN];
#endif

#if USB_SIM_TEST_ENABLE_HID_GAMEPAD
#define USB_SIM_DEVICE_PID 0x0022
#else
#define USB_SIM_DEVICE_PID 0x0021
#endif

static bool usb_sim_enable_ir_live_mouse(void);
static bool usb_sim_enable_ir_live_gamepad(void);
static bool usb_sim_enable_ir_live_any(void);

static int8_t usb_sim_limit_to_i8(int32_t value)
{
    if (value > 127) {
        return 127;
    }
    if (value < -127) {
        return -127;
    }
    return (int8_t)value;
}

static int16_t usb_sim_limit_to_i16(int32_t value)
{
    if (value > 32767) {
        return 32767;
    }
    if (value < -32767) {
        return -32767;
    }
    return (int16_t)value;
}

static bool usb_sim_output_mouse_enabled(void)
{
#if USB_SIM_TEST_OUTPUT_MODE == USB_SIM_TEST_OUTPUT_MODE_GAMEPAD_ONLY
    return false;
#else
    return true;
#endif
}

static bool usb_sim_output_gamepad_enabled(void)
{
#if USB_SIM_TEST_ENABLE_HID_GAMEPAD
#if USB_SIM_TEST_OUTPUT_MODE == USB_SIM_TEST_OUTPUT_MODE_MOUSE_ONLY
    return false;
#else
    return true;
#endif
#else
    return false;
#endif
}

static bool usb_sim_enable_fake_ir(void)
{
    if (usb_sim_enable_ir_live_any()) {
        return false;
    }
#if USB_SIM_TEST_ENABLE_FAKE_IR
    return true;
#else
    return false;
#endif
}

static bool usb_sim_enable_fake_led(void)
{
#if USB_SIM_TEST_ENABLE_FAKE_LED
    return true;
#else
    return false;
#endif
}

static bool usb_sim_enable_script_mouse_move(void)
{
    if (!usb_sim_output_mouse_enabled()) {
        return false;
    }
    if (usb_sim_enable_ir_live_any()) {
        return false;
    }
#if USB_SIM_TEST_ENABLE_SCRIPT_MOUSE_MOVE
    return true;
#else
    return false;
#endif
}

static bool usb_sim_enable_script_mouse_click(void)
{
    if (!usb_sim_output_mouse_enabled()) {
        return false;
    }
    if (usb_sim_enable_ir_live_any()) {
        return false;
    }
#if USB_SIM_TEST_ENABLE_SCRIPT_MOUSE_CLICK
    return true;
#else
    return false;
#endif
}

static bool usb_sim_enable_script_keyboard(void)
{
#if USB_SIM_TEST_ENABLE_SCRIPT_KEYBOARD
    return true;
#else
    return false;
#endif
}

static bool usb_sim_enable_gpio_passthrough(void)
{
#if USB_SIM_TEST_ENABLE_GPIO_PASSTHROUGH
    return true;
#else
    return false;
#endif
}

static bool usb_sim_enable_trigger_ref(void)
{
    if (usb_sim_enable_ir_live_any()) {
        return false;
    }
#if USB_SIM_TEST_ENABLE_TRIGGER_REF
    return true;
#else
    return false;
#endif
}

static bool usb_sim_enable_ir_live_mouse(void)
{
    if (!usb_sim_output_mouse_enabled()) {
        return false;
    }
#if USB_SIM_TEST_ENABLE_IR_LIVE_MOUSE
    return true;
#else
    return false;
#endif
}

static bool usb_sim_enable_ir_live_gamepad(void)
{
#if USB_SIM_TEST_ENABLE_IR_LIVE_GAMEPAD
    return true;
#else
    return false;
#endif
}

static bool usb_sim_enable_ir_live_any(void)
{
    if (usb_sim_output_mouse_enabled() && usb_sim_enable_ir_live_mouse()) {
        return true;
    }
    if (usb_sim_output_gamepad_enabled() && usb_sim_enable_ir_live_gamepad()) {
        return true;
    }
    return false;
}

static bool usb_sim_need_keyboard_path(void)
{
    if (usb_sim_enable_script_keyboard()) {
        return true;
    }
    if (usb_sim_enable_gpio_passthrough()) {
        return true;
    }
    return false;
}

static bool usb_sim_need_mouse_path(void)
{
    if (!usb_sim_output_mouse_enabled()) {
        return false;
    }
    if (usb_sim_enable_script_mouse_move()) {
        return true;
    }
    if (usb_sim_enable_script_mouse_click()) {
        return true;
    }
    if (usb_sim_enable_gpio_passthrough()) {
        return true;
    }
    return false;
}

static bool usb_sim_need_gamepad_path(void)
{
    if (!usb_sim_output_gamepad_enabled()) {
        return false;
    }
    if (usb_sim_enable_fake_ir()) {
        return true;
    }
    if (usb_sim_enable_gpio_passthrough()) {
        return true;
    }
    if (usb_sim_enable_ir_live_gamepad()) {
        return true;
    }
    if (usb_sim_enable_trigger_ref()) {
        return true;
    }
    return false;
}

static void usb_sim_report_log_case(void)
{
    osal_printk("[usb_sim_test] feature_ir=%u feature_led=%u feature_mouse_move=%u feature_mouse_click=%u feature_keyboard=%u feature_gpio=%u feature_trigger_ref=%u feature_gamepad=%u output_mode=%u poll=%u ms debounce=%u ms script_step=%u ms offscreen_mode=%u.\r\n",
        (unsigned int)usb_sim_enable_fake_ir(),
        (unsigned int)usb_sim_enable_fake_led(),
        (unsigned int)usb_sim_enable_script_mouse_move(),
        (unsigned int)usb_sim_enable_script_mouse_click(),
        (unsigned int)usb_sim_enable_script_keyboard(),
        (unsigned int)usb_sim_enable_gpio_passthrough(),
        (unsigned int)usb_sim_enable_trigger_ref(),
        (unsigned int)usb_sim_output_gamepad_enabled(),
        (unsigned int)USB_SIM_TEST_OUTPUT_MODE,
        (unsigned int)USB_SIM_TEST_POLL_MS,
        (unsigned int)USB_SIM_TEST_DEBOUNCE_MS,
        (unsigned int)USB_SIM_TEST_SCRIPT_STEP_MS,
        (unsigned int)USB_SIM_TEST_OFFSCREEN_MODE);
    osal_printk("[usb_sim_test] feature_ir_live_mouse=%u feature_ir_live_gamepad=%u.\r\n",
        (unsigned int)usb_sim_enable_ir_live_mouse(),
        (unsigned int)usb_sim_enable_ir_live_gamepad());
}

static void usb_sim_mouse_send(int8_t dx, int8_t dy, int8_t wheel)
{
    usb_sim_mouse_report_t rpt;
    int32_t ret;

    if (g_usb_sim_ctx.usb_ready == 0U) {
        return;
    }

    rpt.kind = USB_SIM_HID_REPORT_ID_MOUSE;
    rpt.key.d8 = 0;
    rpt.key.b.left_key = g_usb_sim_ctx.report_state.mouse_left;
    rpt.key.b.right_key = g_usb_sim_ctx.report_state.mouse_right;
    rpt.key.b.mid_key = g_usb_sim_ctx.report_state.mouse_middle;
    rpt.x = dx;
    rpt.y = dy;
    rpt.wheel = wheel;

    ret = (int32_t)fhid_send_data(g_usb_sim_ctx.hid_index, (char *)&rpt, USB_SIM_MOUSE_REPORT_LEN);
    if (ret == -1) {
        osal_printk("[usb_sim_test] mouse report send failed.\r\n");
    }
}

static int usb_sim_mouse_send_checked(int8_t dx, int8_t dy, int8_t wheel)
{
    usb_sim_mouse_report_t rpt;
    int32_t ret;

    if (g_usb_sim_ctx.usb_ready == 0U) {
        return -1;
    }

    rpt.kind = USB_SIM_HID_REPORT_ID_MOUSE;
    rpt.key.d8 = 0;
    rpt.key.b.left_key = g_usb_sim_ctx.report_state.mouse_left;
    rpt.key.b.right_key = g_usb_sim_ctx.report_state.mouse_right;
    rpt.key.b.mid_key = g_usb_sim_ctx.report_state.mouse_middle;
    rpt.x = dx;
    rpt.y = dy;
    rpt.wheel = wheel;

    ret = (int32_t)fhid_send_data(g_usb_sim_ctx.hid_index, (char *)&rpt, USB_SIM_MOUSE_REPORT_LEN);
    return (ret < 0) ? -1 : 0;
}

static void usb_sim_keyboard_send(void)
{
    usb_sim_keyboard_report_t rpt;
    uint32_t i;
    int32_t ret;

    if (g_usb_sim_ctx.usb_ready == 0U) {
        return;
    }

    rpt.kind = USB_SIM_HID_REPORT_ID_KEYBOARD;
    rpt.special_key = 0;
    rpt.reserve = 0;
    for (i = 0; i < USB_SIM_MAX_KEYS; i++) {
        rpt.key[i] = 0;
    }
    for (i = 0; (i < g_usb_sim_ctx.report_state.keyboard_key_count) && (i < USB_SIM_MAX_KEYS); i++) {
        rpt.key[i] = g_usb_sim_ctx.report_state.keyboard_keys[i];
    }

    ret = (int32_t)fhid_send_data(g_usb_sim_ctx.hid_index, (char *)&rpt, USB_SIM_KEYBOARD_REPORT_LEN);
    if (ret == -1) {
        osal_printk("[usb_sim_test] keyboard report send failed.\r\n");
    }
}

static int usb_sim_keyboard_send_checked(void)
{
    usb_sim_keyboard_report_t rpt;
    uint32_t i;
    int32_t ret;

    if (g_usb_sim_ctx.usb_ready == 0U) {
        return -1;
    }

    rpt.kind = USB_SIM_HID_REPORT_ID_KEYBOARD;
    rpt.special_key = 0;
    rpt.reserve = 0;
    for (i = 0; i < USB_SIM_MAX_KEYS; i++) {
        rpt.key[i] = 0;
    }
    for (i = 0; (i < g_usb_sim_ctx.report_state.keyboard_key_count) && (i < USB_SIM_MAX_KEYS); i++) {
        rpt.key[i] = g_usb_sim_ctx.report_state.keyboard_keys[i];
    }

    ret = (int32_t)fhid_send_data(g_usb_sim_ctx.hid_index, (char *)&rpt, USB_SIM_KEYBOARD_REPORT_LEN);
    return (ret < 0) ? -1 : 0;
}

static uint8_t usb_sim_gamepad_compose_hat(void)
{
    if (g_usb_sim_ctx.report_state.dpad_up != 0U) {
        if (g_usb_sim_ctx.report_state.dpad_left != 0U) {
            return 7U;
        }
        if (g_usb_sim_ctx.report_state.dpad_right != 0U) {
            return 1U;
        }
        return 0U;
    }

    if (g_usb_sim_ctx.report_state.dpad_down != 0U) {
        if (g_usb_sim_ctx.report_state.dpad_left != 0U) {
            return 5U;
        }
        if (g_usb_sim_ctx.report_state.dpad_right != 0U) {
            return 3U;
        }
        return 4U;
    }

    if (g_usb_sim_ctx.report_state.dpad_left != 0U) {
        return 6U;
    }
    if (g_usb_sim_ctx.report_state.dpad_right != 0U) {
        return 2U;
    }
    return USB_SIM_GAMEPAD_HAT_NEUTRAL;
}

static void usb_sim_gamepad_send(void)
{
    usb_sim_gamepad_report_t rpt;
    int32_t ret;

    if (!usb_sim_output_gamepad_enabled() || g_usb_sim_ctx.usb_ready == 0U) {
        return;
    }

    rpt.kind = USB_SIM_HID_REPORT_ID_GAMEPAD;
    rpt.x = g_usb_sim_ctx.report_state.gamepad_x;
    rpt.y = g_usb_sim_ctx.report_state.gamepad_y;
    rpt.rx = g_usb_sim_ctx.report_state.gamepad_rx;
    rpt.ry = g_usb_sim_ctx.report_state.gamepad_ry;
    rpt.hat = g_usb_sim_ctx.report_state.gamepad_hat;
    rpt.buttons = g_usb_sim_ctx.report_state.gamepad_buttons;

    ret = (int32_t)fhid_send_data(g_usb_sim_ctx.hid_index, (char *)&rpt, USB_SIM_GAMEPAD_REPORT_LEN);
    if (ret == -1) {
        osal_printk("[usb_sim_test] gamepad report send failed.\r\n");
    }
}

static int usb_sim_gamepad_send_checked(void)
{
    usb_sim_gamepad_report_t rpt;
    int32_t ret;

    if (!usb_sim_output_gamepad_enabled() || g_usb_sim_ctx.usb_ready == 0U) {
        return -1;
    }

    rpt.kind = USB_SIM_HID_REPORT_ID_GAMEPAD;
    rpt.x = g_usb_sim_ctx.report_state.gamepad_x;
    rpt.y = g_usb_sim_ctx.report_state.gamepad_y;
    rpt.rx = g_usb_sim_ctx.report_state.gamepad_rx;
    rpt.ry = g_usb_sim_ctx.report_state.gamepad_ry;
    rpt.hat = g_usb_sim_ctx.report_state.gamepad_hat;
    rpt.buttons = g_usb_sim_ctx.report_state.gamepad_buttons;

    ret = (int32_t)fhid_send_data(g_usb_sim_ctx.hid_index, (char *)&rpt, USB_SIM_GAMEPAD_REPORT_LEN);
    return (ret < 0) ? -1 : 0;
}

static void usb_sim_gamepad_set_button(uint8_t button, uint8_t pressed)
{
    uint16_t mask;

    if (!usb_sim_output_gamepad_enabled() || button >= 16U) {
        return;
    }

    mask = (uint16_t)(1U << button);
    if (pressed != 0U) {
        g_usb_sim_ctx.report_state.gamepad_buttons |= mask;
    } else {
        g_usb_sim_ctx.report_state.gamepad_buttons &= (uint16_t)(~mask);
    }
    usb_sim_gamepad_send();
}

static void usb_sim_gamepad_update_dpad(uint8_t dpad_dir, uint8_t pressed)
{
    if (!usb_sim_output_gamepad_enabled()) {
        return;
    }

    switch (dpad_dir) {
        case USB_SIM_DPAD_UP:
            g_usb_sim_ctx.report_state.dpad_up = pressed;
            break;
        case USB_SIM_DPAD_DOWN:
            g_usb_sim_ctx.report_state.dpad_down = pressed;
            break;
        case USB_SIM_DPAD_LEFT:
            g_usb_sim_ctx.report_state.dpad_left = pressed;
            break;
        case USB_SIM_DPAD_RIGHT:
            g_usb_sim_ctx.report_state.dpad_right = pressed;
            break;
        default:
            return;
    }

    g_usb_sim_ctx.report_state.gamepad_hat = usb_sim_gamepad_compose_hat();
    usb_sim_gamepad_send();
}

static int16_t usb_sim_map_abs_axis(uint32_t value, uint32_t max_value)
{
    int64_t scaled;

    if (max_value == 0U) {
        return 0;
    }
    if (value > max_value) {
        value = max_value;
    }

    scaled = ((int64_t)value * 65534LL) / (int64_t)max_value;
    scaled -= 32767LL;
    return usb_sim_limit_to_i16((int32_t)scaled);
}

static void usb_sim_gamepad_set_aim_xy(uint32_t x, uint32_t y, uint32_t max_x, uint32_t max_y)
{
    int16_t mapped_x;
    int16_t mapped_y;

    if (!usb_sim_output_gamepad_enabled()) {
        return;
    }

    mapped_x = usb_sim_map_abs_axis(x, max_x);
    mapped_y = usb_sim_map_abs_axis(y, max_y);
    if (g_usb_sim_ctx.report_state.gamepad_x == mapped_x &&
        g_usb_sim_ctx.report_state.gamepad_y == mapped_y) {
        return;
    }

    g_usb_sim_ctx.report_state.gamepad_x = mapped_x;
    g_usb_sim_ctx.report_state.gamepad_y = mapped_y;
    usb_sim_gamepad_send();
}

static void usb_sim_reset_runtime_state(void)
{
    uint32_t i;

    g_usb_sim_ctx.scripted_elapsed_ms = 0U;
    g_usb_sim_ctx.trigger_mode_elapsed_ms = 0U;
    g_usb_sim_ctx.script_step_index = 0U;
    g_usb_sim_ctx.link_probe_elapsed_ms = 0U;
    g_usb_sim_ctx.link_fail_count = 0U;
    g_usb_sim_ctx.prev_ir_valid = 0U;
    g_usb_sim_ctx.live_ir_prev_valid = 0U;
    g_usb_sim_ctx.current_onscreen = 1U;
    g_usb_sim_ctx.report_state.mouse_left = 0U;
    g_usb_sim_ctx.report_state.mouse_right = 0U;
    g_usb_sim_ctx.report_state.mouse_middle = 0U;
    g_usb_sim_ctx.report_state.keyboard_key_count = 0U;
    g_usb_sim_ctx.report_state.trigger_offscreen_active = 0U;
    g_usb_sim_ctx.report_state.dpad_up = 0U;
    g_usb_sim_ctx.report_state.dpad_down = 0U;
    g_usb_sim_ctx.report_state.dpad_left = 0U;
    g_usb_sim_ctx.report_state.dpad_right = 0U;
    g_usb_sim_ctx.report_state.gamepad_hat = USB_SIM_GAMEPAD_HAT_NEUTRAL;
    g_usb_sim_ctx.report_state.gamepad_buttons = 0U;
    g_usb_sim_ctx.report_state.gamepad_x = 0;
    g_usb_sim_ctx.report_state.gamepad_y = 0;
    g_usb_sim_ctx.report_state.gamepad_rx = 0;
    g_usb_sim_ctx.report_state.gamepad_ry = 0;

    for (i = 0; i < USB_SIM_MAX_KEYS; i++) {
        g_usb_sim_ctx.report_state.keyboard_keys[i] = 0U;
    }

    for (i = 0; i < USB_SIM_LED_COUNT; i++) {
        g_usb_sim_ctx.led_state[i] = 0U;
    }
}

static void usb_sim_mark_inactive(void)
{
    if (g_usb_sim_ctx.usb_active == 0U) {
        return;
    }

    g_usb_sim_ctx.usb_active = 0U;
    usb_sim_reset_runtime_state();
    osal_printk("[usb_sim_test] USB host detached or HID link lost, test flow paused.\r\n");
}

static void usb_sim_mark_active(void)
{
    if (g_usb_sim_ctx.usb_active != 0U) {
        return;
    }

    g_usb_sim_ctx.usb_active = 1U;
    g_usb_sim_ctx.link_fail_count = 0U;
    g_usb_sim_ctx.scripted_elapsed_ms = 0U;
    g_usb_sim_ctx.trigger_mode_elapsed_ms = 0U;
    g_usb_sim_ctx.script_step_index = 0U;
    g_usb_sim_ctx.prev_ir_valid = 0U;
    g_usb_sim_ctx.live_ir_prev_valid = 0U;
    osal_printk("[usb_sim_test] USB host enumerated, test flow activated.\r\n");
}

static int usb_sim_probe_link_once(void)
{
    /*
     * 链路探测优先只探测当前真正启用的 HID 路径。
     * 这样在 scripted mouse move 关闭时，不会让串口看起来还在做鼠标移动业务。
     */
    if (usb_sim_need_keyboard_path()) {
        if (usb_sim_keyboard_send_checked() == 0) {
            return 0;
        }
    }

    if (usb_sim_need_mouse_path()) {
        if (usb_sim_mouse_send_checked(0, 0, 0) == 0) {
            return 0;
        }
    }

    if (usb_sim_need_gamepad_path()) {
        if (usb_sim_gamepad_send_checked() == 0) {
            return 0;
        }
    }

    if (!usb_sim_need_keyboard_path() && !usb_sim_need_mouse_path() && !usb_sim_need_gamepad_path()) {
        if (usb_sim_keyboard_send_checked() == 0) {
            return 0;
        }
        if (usb_sim_mouse_send_checked(0, 0, 0) == 0) {
            return 0;
        }
        if (usb_sim_gamepad_send_checked() == 0) {
            return 0;
        }
    }
    return -1;
}

static void usb_sim_link_guard_update(uint32_t elapsed_ms)
{
    if (g_usb_sim_ctx.usb_ready == 0U) {
        return;
    }

    g_usb_sim_ctx.link_probe_elapsed_ms += elapsed_ms;
    if (g_usb_sim_ctx.link_probe_elapsed_ms < USB_SIM_TEST_SCRIPT_STEP_MS) {
        return;
    }

    g_usb_sim_ctx.link_probe_elapsed_ms = 0U;

    if (usb_sim_probe_link_once() == 0) {
        g_usb_sim_ctx.link_fail_count = 0U;
        usb_sim_mark_active();
        return;
    }

    if (g_usb_sim_ctx.link_fail_count < 0xFFU) {
        g_usb_sim_ctx.link_fail_count++;
    }

    if (g_usb_sim_ctx.link_fail_count >= USB_SIM_LINK_FAIL_THRESHOLD) {
        usb_sim_mark_inactive();
    }
}

static void usb_sim_keyboard_add_key(uint8_t key_code)
{
    uint32_t i;

    for (i = 0; i < g_usb_sim_ctx.report_state.keyboard_key_count; i++) {
        if (g_usb_sim_ctx.report_state.keyboard_keys[i] == key_code) {
            return;
        }
    }

    if (g_usb_sim_ctx.report_state.keyboard_key_count >= USB_SIM_MAX_KEYS) {
        osal_printk("[usb_sim_test] keyboard rollover full, key=0x%02x ignored.\r\n", key_code);
        return;
    }

    g_usb_sim_ctx.report_state.keyboard_keys[g_usb_sim_ctx.report_state.keyboard_key_count] = key_code;
    g_usb_sim_ctx.report_state.keyboard_key_count++;
    usb_sim_keyboard_send();
}

static void usb_sim_keyboard_remove_key(uint8_t key_code)
{
    uint32_t i;

    for (i = 0; i < g_usb_sim_ctx.report_state.keyboard_key_count; i++) {
        if (g_usb_sim_ctx.report_state.keyboard_keys[i] != key_code) {
            continue;
        }

        for (; i + 1 < g_usb_sim_ctx.report_state.keyboard_key_count; i++) {
            g_usb_sim_ctx.report_state.keyboard_keys[i] = g_usb_sim_ctx.report_state.keyboard_keys[i + 1U];
        }
        g_usb_sim_ctx.report_state.keyboard_keys[g_usb_sim_ctx.report_state.keyboard_key_count - 1U] = 0;
        g_usb_sim_ctx.report_state.keyboard_key_count--;
        usb_sim_keyboard_send();
        return;
    }
}

static void usb_sim_set_mouse_button(uint8_t left, uint8_t right, uint8_t middle)
{
    g_usb_sim_ctx.report_state.mouse_left = left;
    g_usb_sim_ctx.report_state.mouse_right = right;
    g_usb_sim_ctx.report_state.mouse_middle = middle;
    if (usb_sim_output_mouse_enabled()) {
        usb_sim_mouse_send(0, 0, 0);
    }
}

static void usb_sim_trigger_press(void)
{
    usb_sim_gamepad_set_button(USB_SIM_GAMEPAD_BTN_PRIMARY, 1U);

    if (g_usb_sim_ctx.current_onscreen != 0U) {
        g_usb_sim_ctx.report_state.trigger_offscreen_active = 0U;
        osal_printk("[usb_sim_test] trigger press -> onscreen fire -> mouse left down.\r\n");
        usb_sim_set_mouse_button(1, g_usb_sim_ctx.report_state.mouse_right, g_usb_sim_ctx.report_state.mouse_middle);
        return;
    }

#if USB_SIM_TEST_OFFSCREEN_MODE == USB_SIM_TEST_OFFSCREEN_RIGHT_CLICK
    g_usb_sim_ctx.report_state.trigger_offscreen_active = 1U;
    osal_printk("[usb_sim_test] trigger press -> offscreen fire -> mouse right down.\r\n");
    usb_sim_set_mouse_button(g_usb_sim_ctx.report_state.mouse_left, 1, g_usb_sim_ctx.report_state.mouse_middle);
#else
    g_usb_sim_ctx.report_state.trigger_offscreen_active = 0U;
    osal_printk("[usb_sim_test] trigger press -> offscreen fire suppressed.\r\n");
#endif
}

static void usb_sim_trigger_release(void)
{
    usb_sim_gamepad_set_button(USB_SIM_GAMEPAD_BTN_PRIMARY, 0U);

    if (g_usb_sim_ctx.report_state.trigger_offscreen_active != 0U) {
        g_usb_sim_ctx.report_state.trigger_offscreen_active = 0U;
        osal_printk("[usb_sim_test] trigger release -> offscreen button up.\r\n");
        usb_sim_set_mouse_button(g_usb_sim_ctx.report_state.mouse_left, 0, g_usb_sim_ctx.report_state.mouse_middle);
        return;
    }

    osal_printk("[usb_sim_test] trigger release -> onscreen button up.\r\n");
    usb_sim_set_mouse_button(0, g_usb_sim_ctx.report_state.mouse_right, g_usb_sim_ctx.report_state.mouse_middle);
}

static void usb_sim_led_log_toggle(uint32_t led_index)
{
    if (led_index >= USB_SIM_LED_COUNT) {
        return;
    }
    g_usb_sim_ctx.led_state[led_index] = (uint8_t)!g_usb_sim_ctx.led_state[led_index];
    osal_printk("[usb_sim_test] LED%u %s\r\n",
        (unsigned int)(led_index + 1U),
        g_usb_sim_ctx.led_state[led_index] ? "ON" : "OFF");
}

static void usb_sim_send_fake_ir_point(const usb_sim_ir_point_t *point)
{
    int32_t dx;
    int32_t dy;
    int8_t send_dx;
    int8_t send_dy;

    if (point == NULL) {
        return;
    }

    if (point->onscreen == 0U) {
        g_usb_sim_ctx.current_onscreen = 0U;
        g_usb_sim_ctx.prev_ir_valid = 0U;
        osal_printk("[usb_sim_test] fake_ir -> OFFSCREEN\r\n");
        return;
    }

    g_usb_sim_ctx.current_onscreen = 1U;
    osal_printk("[usb_sim_test] fake_ir -> onscreen point=(%d,%d)\r\n", point->x, point->y);

    if (g_usb_sim_ctx.prev_ir_valid == 0U) {
        g_usb_sim_ctx.prev_ir_x = (uint32_t)point->x;
        g_usb_sim_ctx.prev_ir_y = (uint32_t)point->y;
        g_usb_sim_ctx.prev_ir_valid = 1U;
        if (usb_sim_output_gamepad_enabled()) {
            usb_sim_gamepad_set_aim_xy((uint32_t)point->x, (uint32_t)point->y,
                USB_SIM_FAKE_IR_AXIS_MAX_X, USB_SIM_FAKE_IR_AXIS_MAX_Y);
        }
        return;
    }

    dx = point->x - (int32_t)g_usb_sim_ctx.prev_ir_x;
    dy = point->y - (int32_t)g_usb_sim_ctx.prev_ir_y;
    send_dx = (int8_t)usb_sim_limit_to_i8(dx);
    send_dy = (int8_t)usb_sim_limit_to_i8(dy);

    g_usb_sim_ctx.prev_ir_x = (uint32_t)point->x;
    g_usb_sim_ctx.prev_ir_y = (uint32_t)point->y;

    if (usb_sim_output_gamepad_enabled()) {
        usb_sim_gamepad_set_aim_xy((uint32_t)point->x, (uint32_t)point->y,
            USB_SIM_FAKE_IR_AXIS_MAX_X, USB_SIM_FAKE_IR_AXIS_MAX_Y);
    }

    /*
     * fake IR 和 trigger 参考模式只负责“生成瞄准状态/坐标”。
     * 真正是否要把这些坐标转成鼠标移动，由 scripted mouse move 单独控制。
     */
    if (usb_sim_enable_script_mouse_move()) {
        usb_sim_mouse_send(send_dx, send_dy, 0);
    }
}

static void usb_sim_update_live_ir_mouse(void)
{
    ir_test_runtime_solution_t solution;
    int32_t dx;
    int32_t dy;

    if (!usb_sim_enable_ir_live_mouse()) {
        return;
    }
    if (ir_test_get_latest_solution(&solution) != 0) {
        return;
    }

    g_usb_sim_ctx.current_onscreen = (solution.valid != 0U && solution.seen_count >= 2U) ? 1U : 0U;
    if (solution.valid == 0U || solution.seen_count < 2U || solution.onscreen == 0U) {
        g_usb_sim_ctx.live_ir_prev_valid = 0U;
        return;
    }

    if (g_usb_sim_ctx.live_ir_prev_valid == 0U) {
        g_usb_sim_ctx.live_ir_last_x = solution.screen_x;
        g_usb_sim_ctx.live_ir_last_y = solution.screen_y;
        g_usb_sim_ctx.live_ir_prev_valid = 1U;
        osal_printk("[usb_sim_test] live ir mouse armed at screen=(%u,%u) seen=%u degraded=%u.\r\n",
            (unsigned int)solution.screen_x,
            (unsigned int)solution.screen_y,
            (unsigned int)solution.seen_count,
            (unsigned int)solution.degraded);
        return;
    }

    dx = (int32_t)solution.screen_x - (int32_t)g_usb_sim_ctx.live_ir_last_x;
    dy = (int32_t)solution.screen_y - (int32_t)g_usb_sim_ctx.live_ir_last_y;
    g_usb_sim_ctx.live_ir_last_x = solution.screen_x;
    g_usb_sim_ctx.live_ir_last_y = solution.screen_y;

    if (dx == 0 && dy == 0) {
        return;
    }

    usb_sim_mouse_send(usb_sim_limit_to_i8(dx), usb_sim_limit_to_i8(dy), 0);
}

static void usb_sim_update_live_ir_gamepad(void)
{
    ir_test_runtime_solution_t solution;

    if (!usb_sim_enable_ir_live_gamepad()) {
        return;
    }
    if (ir_test_get_latest_solution(&solution) != 0) {
        return;
    }

    g_usb_sim_ctx.current_onscreen = (solution.valid != 0U && solution.seen_count >= 2U) ? 1U : 0U;
    if (solution.valid == 0U || solution.seen_count < 2U || solution.onscreen == 0U) {
        return;
    }

    usb_sim_gamepad_set_aim_xy(solution.screen_x, solution.screen_y,
        (uint32_t)(IR_TEST_SCREEN_WIDTH - 1U), (uint32_t)(IR_TEST_SCREEN_HEIGHT - 1U));
}

static void usb_sim_run_script_step(void)
{
    uint32_t ir_index = g_usb_sim_ctx.script_step_index % (sizeof(g_usb_sim_ir_script) / sizeof(g_usb_sim_ir_script[0]));

    if (usb_sim_enable_fake_led()) {
        usb_sim_led_log_toggle(g_usb_sim_ctx.script_step_index % USB_SIM_LED_COUNT);
    }

    if (usb_sim_enable_fake_ir() || usb_sim_enable_trigger_ref()) {
        usb_sim_send_fake_ir_point(&g_usb_sim_ir_script[ir_index]);
    }

    if (usb_sim_enable_script_mouse_click() || usb_sim_enable_script_keyboard()) {
        switch (g_usb_sim_ctx.script_step_index % 6U) {
            case 0:
                if (usb_sim_enable_script_mouse_click()) {
                    osal_printk("[usb_sim_test] script mouse left click.\r\n");
                    usb_sim_set_mouse_button(1, 0, 0);
                }
                break;
            case 1:
                if (usb_sim_enable_script_mouse_click()) {
                    usb_sim_set_mouse_button(0, 0, 0);
                }
                break;
            case 2:
                if (usb_sim_enable_script_keyboard()) {
                    osal_printk("[usb_sim_test] script keyboard ENTER down.\r\n");
                    usb_sim_keyboard_add_key(HID_KEY_ENTER);
                }
                break;
            case 3:
                if (usb_sim_enable_script_keyboard()) {
                    usb_sim_keyboard_remove_key(HID_KEY_ENTER);
                }
                break;
            case 4:
                if (usb_sim_enable_script_keyboard()) {
                    osal_printk("[usb_sim_test] script keyboard ESC down.\r\n");
                    usb_sim_keyboard_add_key(HID_KEY_ESC);
                }
                break;
            default:
                if (usb_sim_enable_script_keyboard()) {
                    usb_sim_keyboard_remove_key(HID_KEY_ESC);
                }
                break;
        }
    }

    g_usb_sim_ctx.script_step_index++;
}

static void usb_sim_buttons_init(void)
{
    uint32_t i;

    uapi_pin_init();
    uapi_gpio_init();

    for (i = 0; i < (sizeof(g_usb_sim_buttons) / sizeof(g_usb_sim_buttons[0])); i++) {
        if (g_usb_sim_buttons[i].pin == (pin_t)USB_SIM_INVALID_PIN) {
            g_usb_sim_buttons[i].enabled = 0U;
            continue;
        }

        g_usb_sim_buttons[i].debounce_ms = 0U;
        (void)uapi_pin_set_ie(g_usb_sim_buttons[i].pin, PIN_IE_1);
        (void)uapi_pin_set_mode(g_usb_sim_buttons[i].pin, HAL_PIO_FUNC_GPIO);
        (void)uapi_pin_set_pull(g_usb_sim_buttons[i].pin, PIN_PULL_UP);
        (void)uapi_gpio_set_dir(g_usb_sim_buttons[i].pin, GPIO_DIRECTION_INPUT);
    }

    osal_printk("[usb_sim_test] gpio inputs ready, default INPUT_PULLUP.\r\n");
}

static uint8_t usb_sim_gpio_read_pressed(pin_t pin)
{
    gpio_level_t level = uapi_gpio_get_val(pin);
    return (level == GPIO_LEVEL_LOW) ? 1U : 0U;
}

static void usb_sim_handle_button_event(usb_sim_button_t *button, uint8_t pressed)
{
    if (button == NULL) {
        return;
    }

    osal_printk("[usb_sim_test] button %-10s %s\r\n", button->name, pressed ? "DOWN" : "UP");

    switch (button->role) {
        case USB_BTN_ROLE_TRIGGER:
            if (pressed != 0U) {
                usb_sim_trigger_press();
            } else {
                usb_sim_trigger_release();
            }
            break;

        case USB_BTN_ROLE_MOUSE_RIGHT:
            usb_sim_set_mouse_button(g_usb_sim_ctx.report_state.mouse_left,
                pressed,
                g_usb_sim_ctx.report_state.mouse_middle);
            usb_sim_gamepad_set_button(button->gamepad_button, pressed);
            break;

        case USB_BTN_ROLE_MOUSE_MIDDLE:
            usb_sim_set_mouse_button(g_usb_sim_ctx.report_state.mouse_left,
                g_usb_sim_ctx.report_state.mouse_right,
                pressed);
            usb_sim_gamepad_set_button(button->gamepad_button, pressed);
            break;

        case USB_BTN_ROLE_KEYBOARD:
            if (pressed != 0U) {
                usb_sim_keyboard_add_key(button->key_code);
            } else {
                usb_sim_keyboard_remove_key(button->key_code);
            }
            if (button->gamepad_button != USB_SIM_GAMEPAD_INVALID_BUTTON) {
                usb_sim_gamepad_set_button(button->gamepad_button, pressed);
            }
            if (button->dpad_dir != USB_SIM_DPAD_NONE) {
                usb_sim_gamepad_update_dpad(button->dpad_dir, pressed);
            }
            break;

        case USB_BTN_ROLE_RESERVED:
        default:
            if (button->gamepad_button != USB_SIM_GAMEPAD_INVALID_BUTTON) {
                usb_sim_gamepad_set_button(button->gamepad_button, pressed);
                break;
            }
            osal_printk("[usb_sim_test] button %s reserved, current test does not bind HID action.\r\n", button->name);
            break;
    }
}

static void usb_sim_scan_buttons(uint32_t elapsed_ms)
{
    uint32_t i;

    if (!usb_sim_enable_gpio_passthrough()) {
        return;
    }

    for (i = 0; i < (sizeof(g_usb_sim_buttons) / sizeof(g_usb_sim_buttons[0])); i++) {
        uint8_t sample;

        if (g_usb_sim_buttons[i].enabled == 0U) {
            continue;
        }

        sample = usb_sim_gpio_read_pressed(g_usb_sim_buttons[i].pin);
        if (sample != g_usb_sim_buttons[i].raw_pressed) {
            g_usb_sim_buttons[i].raw_pressed = sample;
            g_usb_sim_buttons[i].debounce_ms = 0U;
            continue;
        }

        if (g_usb_sim_buttons[i].debounce_ms < USB_SIM_TEST_DEBOUNCE_MS) {
            g_usb_sim_buttons[i].debounce_ms += elapsed_ms;
            continue;
        }

        if (g_usb_sim_buttons[i].stable_pressed == sample) {
            continue;
        }

        g_usb_sim_buttons[i].stable_pressed = sample;
        usb_sim_handle_button_event(&g_usb_sim_buttons[i], sample);
    }
}

static void usb_sim_update_trigger_reference(uint32_t elapsed_ms)
{
    if (!usb_sim_enable_trigger_ref()) {
        return;
    }

    g_usb_sim_ctx.trigger_mode_elapsed_ms += elapsed_ms;
    if (g_usb_sim_ctx.trigger_mode_elapsed_ms < USB_SIM_TEST_TRIGGER_TOGGLE_MS) {
        return;
    }

    g_usb_sim_ctx.trigger_mode_elapsed_ms = 0U;
    g_usb_sim_ctx.current_onscreen = (uint8_t)!g_usb_sim_ctx.current_onscreen;
    g_usb_sim_ctx.prev_ir_valid = 0U;

    osal_printk("[usb_sim_test] trigger reference state -> %s\r\n",
        g_usb_sim_ctx.current_onscreen ? "ONSCREEN" : "OFFSCREEN");
}

static int usb_sim_usb_init(void)
{
    static const char manufacturer[] = { 'T', 0, 'R', 0, '5', 0, '3', 0, '1', 0, 'X', 0 };
    static const char product[] = { 'L', 0, 'i', 0, 'g', 0, 'h', 0, 't', 0, 'G', 0, 'u', 0, 'n', 0, ' ', 0,
                                    'U', 0, 'S', 0, 'B', 0, ' ', 0, 'S', 0, 'i', 0, 'm', 0 };
    static const char serial[] = { 'U', 0, 'S', 0, 'B', 0, '0', 0, '0', 0, '0', 0, '1', 0 };
    struct device_string str_manufacturer = { manufacturer, sizeof(manufacturer) };
    struct device_string str_product = { product, sizeof(product) };
    struct device_string str_serial = { serial, sizeof(serial) };
    struct device_id dev_id = {
        .vendor_id = 0x1111,
        .product_id = USB_SIM_DEVICE_PID,
        .release_num = 0x0100,
    };

    int32_t hid_index = hid_add_report_descriptor(
        g_usb_sim_report_desc_hid,
        sizeof(g_usb_sim_report_desc_hid),
        USB_SIM_HID_PROTOCOL_COMPOSITE);

    if (hid_index < 0) {
        osal_printk("[usb_sim_test] hid_add_report_descriptor failed.\r\n");
        return -1;
    }

    g_usb_sim_ctx.hid_index = (uint8_t)hid_index;

    if (usbd_set_device_info(USB_SIM_DEVICE_TYPE, &str_manufacturer, &str_product, &str_serial, dev_id) != 0) {
        osal_printk("[usb_sim_test] usbd_set_device_info failed.\r\n");
        return -1;
    }

    if (usb_init(DEVICE, USB_SIM_DEVICE_TYPE) != 0) {
        osal_printk("[usb_sim_test] usb_init(%s) failed.\r\n", USB_SIM_DEVICE_MODE_NAME);
        return -1;
    }

    osal_msleep(USB_SIM_HID_INIT_DELAY_MS);
    g_usb_sim_ctx.usb_ready = 1U;
    osal_printk("[usb_sim_test] USB %s ready: hid_index=%u report_ids=(kbd:%u mouse:%u gamepad:%u) pid=0x%04x.\r\n",
        USB_SIM_DEVICE_MODE_NAME,
        (unsigned int)g_usb_sim_ctx.hid_index,
        (unsigned int)USB_SIM_HID_REPORT_ID_KEYBOARD,
        (unsigned int)USB_SIM_HID_REPORT_ID_MOUSE,
        (unsigned int)USB_SIM_HID_REPORT_ID_GAMEPAD,
        (unsigned int)USB_SIM_DEVICE_PID);
    return 0;
}

#if defined(CONFIG_LIGHT_GUN_USB_DEVICE_MODE_ACM_HID) && defined(CONFIG_LIGHT_GUN_USB_CDC_ECHO_TEST)
static int usb_sim_cdc_echo_task(void *arg)
{
    static const char cdc_banner[] = "[usb_sim_test][cdc] echo task ready, send bytes to verify RX/TX.\r\n";
    ssize_t ret;

    unused(arg);

    (void)usb_serial_ioctl(0, CONSOLE_CMD_RD_BLOCK_SERIAL, 1);
    ret = usb_serial_write(0, cdc_banner, sizeof(cdc_banner) - 1U);
    osal_printk("[usb_sim_test] CDC echo task started, banner_write=%d.\r\n", (int)ret);

    while (1) {
        ssize_t recv_len = usb_serial_read(0, g_usb_sim_cdc_echo_buf, sizeof(g_usb_sim_cdc_echo_buf));
        if (recv_len <= 0) {
            osal_msleep(20);
            continue;
        }

        osal_printk("[usb_sim_test] CDC RX %d bytes, echo back.\r\n", (int)recv_len);
        ret = usb_serial_write(0, g_usb_sim_cdc_echo_buf, (size_t)recv_len);
        if (ret < 0) {
            osal_printk("[usb_sim_test] CDC echo write failed, ret=%d.\r\n", (int)ret);
        }
    }

    return 0;
}

static void usb_sim_cdc_echo_start(void)
{
    osal_task *task = osal_kthread_create(usb_sim_cdc_echo_task, NULL, "UsbCdcEcho", USB_SIM_CDC_TASK_STACK_SIZE);
    if (task == NULL) {
        osal_printk("[usb_sim_test] CDC echo task create failed.\r\n");
        return;
    }

    (void)osal_kthread_set_priority(task, USB_SIM_CDC_TASK_PRIO);
}
#else
static void usb_sim_cdc_echo_start(void)
{
}
#endif

static int usb_sim_test_task(void *arg)
{
    unused(arg);

    usb_sim_report_log_case();
    osal_printk("[usb_sim_test] boot delay %u ms for USB enumerate / probe hookup.\r\n",
        (unsigned int)USB_SIM_TEST_START_DELAY_MS);

    if (usb_sim_usb_init() != 0) {
        return -1;
    }
    usb_sim_cdc_echo_start();

    usb_sim_reset_runtime_state();
    usb_sim_buttons_init();
    osal_msleep(USB_SIM_TEST_START_DELAY_MS);
    osal_printk("[usb_sim_test] waiting for USB host enumeration before starting tests.\r\n");

    while (1) {
        usb_sim_link_guard_update(USB_SIM_TEST_POLL_MS);

        if (g_usb_sim_ctx.usb_active == 0U) {
            osal_msleep(USB_SIM_TEST_POLL_MS);
            continue;
        }

        usb_sim_scan_buttons(USB_SIM_TEST_POLL_MS);
        usb_sim_update_trigger_reference(USB_SIM_TEST_POLL_MS);
        usb_sim_update_live_ir_mouse();
        usb_sim_update_live_ir_gamepad();

        g_usb_sim_ctx.scripted_elapsed_ms += USB_SIM_TEST_POLL_MS;
        if (g_usb_sim_ctx.scripted_elapsed_ms >= USB_SIM_TEST_SCRIPT_STEP_MS) {
            g_usb_sim_ctx.scripted_elapsed_ms = 0U;
            usb_sim_run_script_step();
        }

        osal_msleep(USB_SIM_TEST_POLL_MS);
    }

    return 0;
}

void usb_sim_test_overlay_entry(void)
{
    osal_task *task = osal_kthread_create(usb_sim_test_task, NULL, "UsbSimTest", USB_SIM_TASK_STACK_SIZE);
    if (task == NULL) {
        osal_printk("[usb_sim_test] task create failed.\r\n");
        return;
    }

    (void)osal_kthread_set_priority(task, USB_SIM_TASK_PRIO);
}
