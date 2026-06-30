#include "drivers/drv_usb_hid.h"

#include <stdint.h>
#include <string.h>

#include "common_def.h"
#include "gadget/f_hid.h"
#include "implementation/usb_init.h"
#include "of_wireless_pkt.h"
#include "osal_debug.h"
#include "soc_osal.h"

#define OF_USB_HID_ENABLE 1

#ifndef OF_USB_HID_INIT_DELAY_MS
#ifdef CONFIG_LIGHT_GUN_260517_USB_HID_INIT_DELAY_MS
#define OF_USB_HID_INIT_DELAY_MS CONFIG_LIGHT_GUN_260517_USB_HID_INIT_DELAY_MS
#else
#define OF_USB_HID_INIT_DELAY_MS 500U
#endif
#endif

#define USB_HID_INIT_DELAY_MS 500U
#define USB_HID_KEYBOARD_REPORT_LEN 9U
#define USB_HID_MOUSE_REPORT_LEN 5U
#define USB_HID_GAMEPAD_REPORT_LEN 18U
#define USB_HID_MAX_KEYS 6U

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

typedef union {
    struct {
        uint8_t left_key : 1;
        uint8_t right_key : 1;
        uint8_t mid_key : 1;
        uint8_t reserved : 5;
    } b;
    uint8_t d8;
} usb_hid_mouse_key_t;

typedef struct {
    uint8_t kind;
    usb_hid_mouse_key_t key;
    int8_t x;
    int8_t y;
    int8_t wheel;
} usb_hid_mouse_report_t;

typedef struct {
    uint8_t kind;
    uint8_t special_key;
    uint8_t reserve;
    uint8_t key[USB_HID_MAX_KEYS];
} usb_hid_keyboard_report_t;

typedef struct __attribute__((packed)) {
    uint8_t kind;
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t rz;
    int16_t rx;
    int16_t ry;
    uint8_t hat;
    uint32_t buttons;
} usb_hid_gamepad_report_t;

static const uint8_t g_usb_hid_report_desc_keyboard[] = {
    usage_page(1),      0x01,
    usage(1),           0x06,
    collection(1),      0x01,
    report_id(1),       0x01,
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
};

static const uint8_t g_usb_hid_report_desc_mouse[] = {
    usage_page(1),      0x01,
    usage(1),           0x02,
    collection(1),      0x01,
    report_id(1),       0x04,
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
};

static const uint8_t g_usb_hid_report_desc_gamepad[] = {
    usage_page(1),      0x01,
    usage(1),           0x05,
    collection(1),      0x01,
    report_id(1),       0x03,
    usage_page(1),      0x01,
    usage(1),           0x30,
    usage(1),           0x31,
    usage(1),           0x32,
    usage(1),           0x35,
    usage(1),           0x33,
    usage(1),           0x34,
    logical_minimum(2), 0x01, 0x80,
    logical_maximum(2), 0xFF, 0x7F,
    report_count(1),    0x06,
    report_size(1),     0x10,
    input(1),           0x02,
    usage_page(1),      0x01,
    usage(1),           0x39,
    logical_minimum(1), 0x01,
    logical_maximum(1), 0x08,
    0x35,               0x00,
    0x46,               0x3B, 0x01,
    report_count(1),    0x01,
    report_size(1),     0x08,
    input(1),           0x02,
    usage_page(1),      0x09,
    usage_minimum(1),   0x01,
    usage_maximum(1),   0x20,
    logical_minimum(1), 0x00,
    logical_maximum(1), 0x01,
    report_count(1),    0x20,
    report_size(1),     0x01,
    input(1),           0x02,
    end_collection(0),
};

static uint8_t g_usb_hid_inited;
static uint8_t g_usb_hid_ready;
static uint8_t g_usb_hid_keyboard_index;
static uint8_t g_usb_hid_mouse_index;
static uint8_t g_usb_hid_gamepad_index;

static int usb_hid_send_mouse_raw(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel)
{
    usb_hid_mouse_report_t rpt;
    int32_t ret;

    if (g_usb_hid_inited == 0U) {
        return -1;
    }

    rpt.kind = 0x04U;
    rpt.key.d8 = buttons;
    rpt.x = dx;
    rpt.y = dy;
    rpt.wheel = wheel;

    ret = (int32_t)fhid_send_data(g_usb_hid_mouse_index, (char *)&rpt, USB_HID_MOUSE_REPORT_LEN);
    return (ret < 0) ? -1 : 0;
}

static int usb_hid_send_keyboard_raw(const uint8_t *keys, uint8_t key_count)
{
    usb_hid_keyboard_report_t rpt;
    uint8_t i;
    int32_t ret;

    if (g_usb_hid_inited == 0U) {
        return -1;
    }

    (void)memset(&rpt, 0, sizeof(rpt));
    rpt.kind = 0x01U;
    for (i = 0U; (i < key_count) && (i < USB_HID_MAX_KEYS); i++) {
        rpt.key[i] = keys[i];
    }

    ret = (int32_t)fhid_send_data(g_usb_hid_keyboard_index, (char *)&rpt, USB_HID_KEYBOARD_REPORT_LEN);
    return (ret < 0) ? -1 : 0;
}

static int usb_hid_send_gamepad_raw(const void *report, uint32_t report_len)
{
    usb_hid_gamepad_report_t rpt;
    int32_t ret;

    if ((report == 0) || (report_len != sizeof(of_wpkt_gamepad_payload_t))) {
        return -1;
    }
    if (g_usb_hid_inited == 0U) {
        return -1;
    }

    (void)memset(&rpt, 0, sizeof(rpt));
    rpt.kind = 0x03U;
    (void)memcpy(&rpt.x, report, report_len);

    ret = (int32_t)fhid_send_data(g_usb_hid_gamepad_index, (char *)&rpt, USB_HID_GAMEPAD_REPORT_LEN);
    return (ret < 0) ? -1 : 0;
}

int drv_usb_hid_init(void)
{
    const char manufacturer[] = { 'O', 0, 'B', 0, 'T', 0 };
    const char product[] = { 'L', 0, 'G', 0, 'D', 0, 'o', 0, 'n', 0, 'g', 0, 'l', 0, 'e', 0, '2', 0, '6', 0, '0', 0, '5', 0, '1', 0, '7', 0 };
    const char serial[] = { 'D', 0, 'N', 0, 'G', 0, '2', 0, '6', 0, '0', 0, '5', 0, '1', 0, '7', 0 };
    struct device_string str_manufacturer = { manufacturer, sizeof(manufacturer) };
    struct device_string str_product = { product, sizeof(product) };
    struct device_string str_serial_number = { serial, sizeof(serial) };
    struct device_id dev_id = {
        .vendor_id = 0x1111,
        .product_id = 0x2606,
        .release_num = 0x0100
    };

    if (!OF_USB_HID_ENABLE) {
        return -1;
    }

    if (g_usb_hid_inited != 0U) {
        return 0;
    }

    g_usb_hid_keyboard_index = (uint8_t)hid_add_report_descriptor(
        g_usb_hid_report_desc_keyboard, sizeof(g_usb_hid_report_desc_keyboard), 1);
    g_usb_hid_mouse_index = (uint8_t)hid_add_report_descriptor(
        g_usb_hid_report_desc_mouse, sizeof(g_usb_hid_report_desc_mouse), 2);
    g_usb_hid_gamepad_index = (uint8_t)hid_add_report_descriptor(
        g_usb_hid_report_desc_gamepad, sizeof(g_usb_hid_report_desc_gamepad), 3);

    if (usbd_set_device_info(DEV_HID, &str_manufacturer, &str_product, &str_serial_number, dev_id) != 0) {
        osal_printk("[drv_usb_hid] usbd_set_device_info failed.\r\n");
        return -1;
    }
    if (usb_init(DEVICE, DEV_HID) != 0) {
        osal_printk("[drv_usb_hid] usb_init DEV_HID failed.\r\n");
        return -1;
    }

    osal_msleep(OF_USB_HID_INIT_DELAY_MS);
    g_usb_hid_inited = 1U;
    g_usb_hid_ready = 0U;
    osal_printk("[drv_usb_hid] init ok, keyboard_index=%u mouse_index=%u gamepad_index=%u.\r\n",
        (unsigned int)g_usb_hid_keyboard_index, (unsigned int)g_usb_hid_mouse_index,
        (unsigned int)g_usb_hid_gamepad_index);
    return 0;
}

void drv_usb_hid_deinit(void)
{
    g_usb_hid_ready = 0U;
}

int drv_usb_hid_is_ready(void)
{
    return (g_usb_hid_ready != 0U);
}

void drv_usb_hid_set_ready(int ready)
{
    g_usb_hid_ready = (ready != 0) ? 1U : 0U;
}

int drv_usb_hid_send_mouse_report(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel)
{
    if (g_usb_hid_ready == 0U) {
        return -1;
    }
    return usb_hid_send_mouse_raw(buttons, dx, dy, wheel);
}

int drv_usb_hid_send_keyboard_report(const uint8_t *keys, uint8_t key_count)
{
    if (g_usb_hid_ready == 0U) {
        return -1;
    }
    return usb_hid_send_keyboard_raw(keys, key_count);
}

int drv_usb_hid_send_gamepad_report(const void *report, uint32_t report_len)
{
    if (g_usb_hid_ready == 0U) {
        return -1;
    }
    return usb_hid_send_gamepad_raw(report, report_len);
}

int drv_usb_hid_probe_ready(void)
{
    static const uint8_t empty_keys[1] = {0};

    if (usb_hid_send_keyboard_raw(empty_keys, 0U) == 0) {
        return 0;
    }
    return usb_hid_send_mouse_raw(0U, 0, 0, 0);
}
