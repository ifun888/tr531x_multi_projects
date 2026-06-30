#include "services/svc_usb_hid.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "drivers/drv_input_keys.h"
#include "drivers/drv_ir_cam.h"
#include "drivers/drv_sle_link.h"
#include "drivers/drv_usb_hid.h"
#include "of_link_io.h"
#include "of_fops.h"
#include "of_transport.h"
#include "osal_debug.h"
#include "platform/of_time.h"
#include "services/svc_position.h"
#include "of_wireless_pkt.h"

#ifndef OF_USB_HID_PROBE_INTERVAL_MS
#ifdef CONFIG_LIGHT_GUN_260517_USB_HID_PROBE_INTERVAL_MS
#define OF_USB_HID_PROBE_INTERVAL_MS CONFIG_LIGHT_GUN_260517_USB_HID_PROBE_INTERVAL_MS
#else
#define OF_USB_HID_PROBE_INTERVAL_MS 250U
#endif
#endif

#ifndef OF_USB_HID_LINK_FAIL_THRESHOLD
#ifdef CONFIG_LIGHT_GUN_260517_USB_HID_LINK_FAIL_THRESHOLD
#define OF_USB_HID_LINK_FAIL_THRESHOLD CONFIG_LIGHT_GUN_260517_USB_HID_LINK_FAIL_THRESHOLD
#else
#define OF_USB_HID_LINK_FAIL_THRESHOLD 3U
#endif
#endif

#define HID_KEY_B      0x05U
#define HID_KEY_ENTER  0x28U
#define HID_KEY_ESC    0x29U
#define HID_KEY_HOME   0x4AU
#define HID_KEY_RIGHT  0x4FU
#define HID_KEY_LEFT   0x50U
#define HID_KEY_DOWN   0x51U
#define HID_KEY_UP     0x52U

typedef struct {
    uint8_t active;
    uint8_t link_fail_count;
    uint8_t probed_once;
    uint8_t mouse_buttons;
    uint8_t key_count;
    uint8_t keyboard_keys[6];
    uint16_t prev_x;
    uint16_t prev_y;
    uint16_t prev_keys;
    uint8_t prev_valid;
    uint32_t probe_last_ms;
} hid_ctx_t;

static hid_ctx_t g_hid;

static int8_t hid_clamp_i8(int32_t value)
{
    if (value > 127) {
        return 127;
    }
    if (value < -127) {
        return -127;
    }
    return (int8_t)value;
}

static void hid_add_key(uint8_t *keys, uint8_t *count, uint8_t key)
{
    uint8_t i;

    for (i = 0U; i < *count; i++) {
        if (keys[i] == key) {
            return;
        }
    }
    if (*count < 6U) {
        keys[*count] = key;
        (*count)++;
    }
}

static void hid_build_keyboard_report(uint16_t keys_mask, uint8_t *keys, uint8_t *key_count)
{
    *key_count = 0U;
    (void)memset(keys, 0, 6U);

    if ((keys_mask & OF_KEY_MASK_START) != 0U) {
        hid_add_key(keys, key_count, HID_KEY_ENTER);
    }
    if ((keys_mask & OF_KEY_MASK_SELECT) != 0U) {
        hid_add_key(keys, key_count, HID_KEY_ESC);
    }
    if ((keys_mask & OF_KEY_MASK_HOME) != 0U) {
        hid_add_key(keys, key_count, HID_KEY_HOME);
    }
    if ((keys_mask & OF_KEY_MASK_UP) != 0U) {
        hid_add_key(keys, key_count, HID_KEY_UP);
    }
    if ((keys_mask & OF_KEY_MASK_DOWN) != 0U) {
        hid_add_key(keys, key_count, HID_KEY_DOWN);
    }
    if ((keys_mask & OF_KEY_MASK_LEFT) != 0U) {
        hid_add_key(keys, key_count, HID_KEY_LEFT);
    }
    if ((keys_mask & OF_KEY_MASK_RIGHT) != 0U) {
        hid_add_key(keys, key_count, HID_KEY_RIGHT);
    }
    if ((keys_mask & OF_KEY_MASK_MIDDLE) != 0U) {
        hid_add_key(keys, key_count, HID_KEY_B);
    }
}

void svc_usb_hid_init(void)
{
    (void)memset(&g_hid, 0, sizeof(g_hid));
}

void svc_usb_hid_tick(void)
{
    const of_dev_t *keys_dev;
    of_pos_sample_t pos;
    uint8_t key_buf[2] = {0};
    uint32_t got = 0U;
    uint16_t keys_mask;
    uint8_t keyboard_keys[6];
    uint8_t keyboard_count;
    uint8_t mouse_buttons = 0U;
    int32_t dx = 0;
    int32_t dy = 0;
    uint32_t now_ms = (uint32_t)(of_time_us() / 1000U);
    int wireless_active = (of_transport_get_type() == OF_TRANSPORT_SLE) && drv_sle_link_is_ready();

    if (!wireless_active && !drv_usb_hid_is_ready()) {
        if ((now_ms - g_hid.probe_last_ms) < OF_USB_HID_PROBE_INTERVAL_MS) {
            return;
        }
        g_hid.probe_last_ms = now_ms;
        if (drv_usb_hid_probe_ready() == 0) {
            drv_usb_hid_set_ready(1);
            g_hid.active = 1U;
            g_hid.link_fail_count = 0U;
            if (g_hid.probed_once == 0U) {
                osal_printk("[svc_usb_hid] HID probe success, report path enabled.\r\n");
            } else {
                osal_printk("[svc_usb_hid] HID link recovered, report path resumed.\r\n");
            }
            g_hid.probed_once = 1U;
        }
        return;
    }

    keys_dev = drv_input_keys_get_dev();
    if ((keys_dev != 0) && (keys_dev->ops != 0) && (keys_dev->ops->read != 0)) {
        (void)keys_dev->ops->read(keys_dev->priv, key_buf, sizeof(key_buf), &got);
    }
    keys_mask = (uint16_t)key_buf[0] | ((uint16_t)key_buf[1] << 8);

    (void)svc_position_get(&pos);

    if ((pos.valid != 0U) && (pos.seen_count >= 2U)) {
        if (g_hid.prev_valid != 0U) {
            dx = (int32_t)pos.x - (int32_t)g_hid.prev_x;
            dy = (int32_t)pos.y - (int32_t)g_hid.prev_y;
        }
        g_hid.prev_x = pos.x;
        g_hid.prev_y = pos.y;
        g_hid.prev_valid = 1U;
    } else {
        g_hid.prev_valid = 0U;
    }

    if ((keys_mask & OF_KEY_MASK_TRIGGER) != 0U) {
        if ((pos.valid != 0U) && (pos.seen_count >= 2U)) {
            mouse_buttons |= 0x01U;
        } else {
            mouse_buttons |= 0x02U;
        }
    }
    if ((keys_mask & OF_KEY_MASK_A) != 0U) {
        mouse_buttons |= 0x02U;
    }
    if ((keys_mask & OF_KEY_MASK_B) != 0U) {
        mouse_buttons |= 0x04U;
    }

    hid_build_keyboard_report(keys_mask, keyboard_keys, &keyboard_count);

    if ((dx != 0) || (dy != 0) || (mouse_buttons != g_hid.mouse_buttons)) {
        int rc;
        if (wireless_active) {
            of_wpkt_mouse_payload_t pkt = {
                .buttons = mouse_buttons,
                .dx = hid_clamp_i8(dx),
                .dy = hid_clamp_i8(dy),
                .wheel = 0
            };
            uint32_t sent = 0U;
            rc = of_link_send_packet(OF_WPKT_TYPE_HID_MOUSE, (const uint8_t *)&pkt, sizeof(pkt), &sent);
        } else {
            rc = drv_usb_hid_send_mouse_report(mouse_buttons, hid_clamp_i8(dx), hid_clamp_i8(dy), 0);
        }
        if (rc == 0) {
            g_hid.active = 1U;
            g_hid.link_fail_count = 0U;
            g_hid.mouse_buttons = mouse_buttons;
            osal_printk("[svc_usb_hid] mouse dx=%d dy=%d buttons=0x%02x pos_valid=%u.\r\n",
                (int)hid_clamp_i8(dx), (int)hid_clamp_i8(dy),
                (unsigned int)mouse_buttons, (unsigned int)pos.valid);
        } else if (g_hid.link_fail_count < 0xFFU) {
            g_hid.link_fail_count++;
            osal_printk("[svc_usb_hid] mouse report send failed, dx=%d dy=%d buttons=0x%02x.\r\n",
                (int)hid_clamp_i8(dx), (int)hid_clamp_i8(dy), (unsigned int)mouse_buttons);
        }
    }

    if ((keyboard_count != g_hid.key_count) ||
        (memcmp(keyboard_keys, g_hid.keyboard_keys, sizeof(keyboard_keys)) != 0)) {
        int rc;
        if (wireless_active) {
            of_wpkt_keyboard_payload_t pkt;
            uint32_t sent = 0U;

            (void)memset(&pkt, 0, sizeof(pkt));
            pkt.key_count = keyboard_count;
            (void)memcpy(pkt.keys, keyboard_keys, sizeof(pkt.keys));
            rc = of_link_send_packet(OF_WPKT_TYPE_HID_KEYBOARD, (const uint8_t *)&pkt, sizeof(pkt), &sent);
        } else {
            rc = drv_usb_hid_send_keyboard_report(keyboard_keys, keyboard_count);
        }
        if (rc == 0) {
            g_hid.active = 1U;
            g_hid.link_fail_count = 0U;
            g_hid.key_count = keyboard_count;
            (void)memcpy(g_hid.keyboard_keys, keyboard_keys, sizeof(g_hid.keyboard_keys));
            osal_printk("[svc_usb_hid] keyboard keys=%u mask=0x%04x.\r\n",
                (unsigned int)keyboard_count, (unsigned int)keys_mask);
        } else if (g_hid.link_fail_count < 0xFFU) {
            g_hid.link_fail_count++;
            osal_printk("[svc_usb_hid] keyboard report send failed, keys=%u mask=0x%04x.\r\n",
                (unsigned int)keyboard_count, (unsigned int)keys_mask);
        }
    }

    if (!wireless_active && (g_hid.link_fail_count >= OF_USB_HID_LINK_FAIL_THRESHOLD)) {
        drv_usb_hid_set_ready(0);
        g_hid.active = 0U;
        g_hid.link_fail_count = 0U;
        g_hid.mouse_buttons = 0U;
        g_hid.key_count = 0U;
        (void)memset(g_hid.keyboard_keys, 0, sizeof(g_hid.keyboard_keys));
        osal_printk("[svc_usb_hid] HID link lost, pause reports and wait re-probe.\r\n");
    }

    g_hid.prev_keys = keys_mask;
}
