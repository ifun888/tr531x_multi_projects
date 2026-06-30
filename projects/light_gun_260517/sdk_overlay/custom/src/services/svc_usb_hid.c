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
#include "of_protocol.h"
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

#define OF_GPAD_HAT_CENTERED   0U
#define OF_GPAD_HAT_UP         1U
#define OF_GPAD_HAT_UP_RIGHT   2U
#define OF_GPAD_HAT_RIGHT      3U
#define OF_GPAD_HAT_DOWN_RIGHT 4U
#define OF_GPAD_HAT_DOWN       5U
#define OF_GPAD_HAT_DOWN_LEFT  6U
#define OF_GPAD_HAT_LEFT       7U
#define OF_GPAD_HAT_UP_LEFT    8U

#define OF_GPAD_BTN_A      0U
#define OF_GPAD_BTN_B      1U
#define OF_GPAD_BTN_X      3U
#define OF_GPAD_BTN_Y      4U
#define OF_GPAD_BTN_SELECT 10U
#define OF_GPAD_BTN_START  11U
#define OF_GPAD_BTN_HOME   12U
#define OF_POS_SCREEN_MAX_X 1919U
#define OF_POS_SCREEN_MAX_Y 1079U

typedef struct {
    uint8_t active;
    uint8_t link_fail_count;
    uint8_t probed_once;
    uint8_t route_wireless_active;
    uint8_t route_wireless_gamepad_mode;
    uint8_t mouse_buttons;
    uint8_t key_count;
    uint8_t keyboard_keys[6];
    of_wpkt_gamepad_payload_t gamepad_report;
    uint16_t prev_x;
    uint16_t prev_y;
    uint16_t prev_keys;
    uint8_t prev_valid;
    uint32_t probe_last_ms;
} hid_ctx_t;

static hid_ctx_t g_hid;

static void hid_reset_cached_reports(void)
{
    g_hid.mouse_buttons = 0U;
    g_hid.key_count = 0U;
    (void)memset(g_hid.keyboard_keys, 0, sizeof(g_hid.keyboard_keys));
    (void)memset(&g_hid.gamepad_report, 0, sizeof(g_hid.gamepad_report));
}

static void hid_reset_motion_history(void)
{
    g_hid.prev_x = 0U;
    g_hid.prev_y = 0U;
    g_hid.prev_keys = 0U;
    g_hid.prev_valid = 0U;
}

static int hid_gamepad_report_is_zero(const of_wpkt_gamepad_payload_t *pkt)
{
    static const of_wpkt_gamepad_payload_t zero_pkt = {0};

    if (pkt == 0) {
        return 1;
    }
    return (memcmp(pkt, &zero_pkt, sizeof(zero_pkt)) == 0);
}

static void hid_release_previous_wireless_family(uint8_t previous_gamepad_mode)
{
    static const uint8_t zero_keys[6] = {0};
    uint32_t sent = 0U;

    if (previous_gamepad_mode != 0U) {
        of_wpkt_gamepad_payload_t pkt = {0};

        if (!hid_gamepad_report_is_zero(&g_hid.gamepad_report)) {
            if (of_link_send_packet(OF_WPKT_TYPE_HID_GAMEPAD, (const uint8_t *)&pkt, sizeof(pkt), &sent) == 0) {
                osal_printk("[svc_usb_hid] released wireless gamepad state on route transition.\r\n");
            }
        }
        return;
    }

    if (g_hid.mouse_buttons != 0U) {
        of_wpkt_mouse_payload_t mouse_pkt = {0};

        sent = 0U;
        if (of_link_send_packet(OF_WPKT_TYPE_HID_MOUSE, (const uint8_t *)&mouse_pkt, sizeof(mouse_pkt), &sent) == 0) {
            osal_printk("[svc_usb_hid] released wireless mouse buttons on route transition.\r\n");
        }
    }

    if ((g_hid.key_count != 0U) || (memcmp(g_hid.keyboard_keys, zero_keys, sizeof(g_hid.keyboard_keys)) != 0)) {
        of_wpkt_keyboard_payload_t key_pkt;

        (void)memset(&key_pkt, 0, sizeof(key_pkt));
        sent = 0U;
        if (of_link_send_packet(OF_WPKT_TYPE_HID_KEYBOARD, (const uint8_t *)&key_pkt, sizeof(key_pkt), &sent) == 0) {
            osal_printk("[svc_usb_hid] released wireless keyboard state on route transition.\r\n");
        }
    }
}

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

static int16_t hid_map_axis_u16(uint16_t value, uint16_t max_value)
{
    int32_t scaled;

    if (value >= max_value) {
        return 32767;
    }
    scaled = ((int32_t)value * 65534) / (int32_t)max_value;
    return (int16_t)(scaled - 32767);
}

static uint8_t hid_build_gamepad_hat(uint16_t keys_mask)
{
    uint8_t up = ((keys_mask & OF_KEY_MASK_UP) != 0U) ? 1U : 0U;
    uint8_t down = ((keys_mask & OF_KEY_MASK_DOWN) != 0U) ? 1U : 0U;
    uint8_t left = ((keys_mask & OF_KEY_MASK_LEFT) != 0U) ? 1U : 0U;
    uint8_t right = ((keys_mask & OF_KEY_MASK_RIGHT) != 0U) ? 1U : 0U;

    if (up && right && !down && !left) {
        return OF_GPAD_HAT_UP_RIGHT;
    }
    if (down && right && !up && !left) {
        return OF_GPAD_HAT_DOWN_RIGHT;
    }
    if (down && left && !up && !right) {
        return OF_GPAD_HAT_DOWN_LEFT;
    }
    if (up && left && !down && !right) {
        return OF_GPAD_HAT_UP_LEFT;
    }
    if (up && !down) {
        return OF_GPAD_HAT_UP;
    }
    if (down && !up) {
        return OF_GPAD_HAT_DOWN;
    }
    if (left && !right) {
        return OF_GPAD_HAT_LEFT;
    }
    if (right && !left) {
        return OF_GPAD_HAT_RIGHT;
    }
    return OF_GPAD_HAT_CENTERED;
}

static uint32_t hid_build_gamepad_buttons(uint16_t keys_mask, uint8_t aim_valid)
{
    uint32_t buttons = 0U;
    uint8_t offscreen_button_enabled = (of_proto_mh_offscreen_button_enabled() != 0) ? 1U : 0U;

    if ((keys_mask & OF_KEY_MASK_TRIGGER) != 0U) {
        if (aim_valid != 0U) {
            buttons |= (1UL << OF_GPAD_BTN_A);
        } else if (offscreen_button_enabled != 0U) {
            buttons |= (1UL << OF_GPAD_BTN_B);
        }
    }
    if ((keys_mask & OF_KEY_MASK_A) != 0U) {
        buttons |= (1UL << OF_GPAD_BTN_B);
    }
    if ((keys_mask & OF_KEY_MASK_B) != 0U) {
        buttons |= (1UL << OF_GPAD_BTN_X);
    }
    if ((keys_mask & OF_KEY_MASK_MIDDLE) != 0U) {
        buttons |= (1UL << OF_GPAD_BTN_Y);
    }
    if ((keys_mask & OF_KEY_MASK_SELECT) != 0U) {
        buttons |= (1UL << OF_GPAD_BTN_SELECT);
    }
    if ((keys_mask & OF_KEY_MASK_START) != 0U) {
        buttons |= (1UL << OF_GPAD_BTN_START);
    }
    if ((keys_mask & OF_KEY_MASK_HOME) != 0U) {
        buttons |= (1UL << OF_GPAD_BTN_HOME);
    }
    return buttons;
}

static void hid_build_gamepad_report(of_wpkt_gamepad_payload_t *pkt, const of_pos_sample_t *pos, uint16_t keys_mask)
{
    int use_left_stick = of_proto_mh_gamepad_aim_with_left_stick();
    uint8_t aim_valid = ((pos->valid != 0U) && (pos->seen_count >= 2U)) ? 1U : 0U;

    (void)memset(pkt, 0, sizeof(*pkt));
    pkt->hat = hid_build_gamepad_hat(keys_mask);
    pkt->buttons = hid_build_gamepad_buttons(keys_mask, aim_valid);

    if (aim_valid == 0U) {
        return;
    }

    if (use_left_stick) {
        pkt->x = hid_map_axis_u16(pos->x, OF_POS_SCREEN_MAX_X);
        pkt->y = hid_map_axis_u16(pos->y, OF_POS_SCREEN_MAX_Y);
    } else {
        pkt->rx = hid_map_axis_u16(pos->x, OF_POS_SCREEN_MAX_X);
        pkt->ry = hid_map_axis_u16(pos->y, OF_POS_SCREEN_MAX_Y);
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
    uint8_t aim_valid = 0U;
    uint8_t offscreen_button_enabled = (of_proto_mh_offscreen_button_enabled() != 0) ? 1U : 0U;
    int32_t dx = 0;
    int32_t dy = 0;
    uint32_t now_ms = (uint32_t)(of_time_us() / 1000U);
    int wireless_link_up = (of_transport_get_type() == OF_TRANSPORT_SLE) && drv_sle_link_is_ready();
    int wireless_active = wireless_link_up && of_link_is_ready();
    int wireless_gamepad_mode = wireless_active && of_proto_mh_gamepad_enabled();
    uint8_t route_changed = ((uint8_t)wireless_active != g_hid.route_wireless_active) ||
        ((uint8_t)wireless_gamepad_mode != g_hid.route_wireless_gamepad_mode);

    if (route_changed != 0U) {
        if (g_hid.route_wireless_active != 0U) {
            hid_release_previous_wireless_family(g_hid.route_wireless_gamepad_mode);
        }
        hid_reset_cached_reports();
        hid_reset_motion_history();
        g_hid.route_wireless_active = (uint8_t)wireless_active;
        g_hid.route_wireless_gamepad_mode = (uint8_t)wireless_gamepad_mode;
    }

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
        aim_valid = 1U;
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
        if (aim_valid != 0U) {
            mouse_buttons |= 0x01U;
        } else if (offscreen_button_enabled != 0U) {
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

    if (wireless_gamepad_mode) {
        of_wpkt_gamepad_payload_t pkt;

        hid_build_gamepad_report(&pkt, &pos, keys_mask);
        if (memcmp(&pkt, &g_hid.gamepad_report, sizeof(pkt)) != 0) {
            uint32_t sent = 0U;
            int rc = of_link_send_packet(OF_WPKT_TYPE_HID_GAMEPAD, (const uint8_t *)&pkt, sizeof(pkt), &sent);

            if (rc == 0) {
                g_hid.active = 1U;
                g_hid.link_fail_count = 0U;
                g_hid.gamepad_report = pkt;
                g_hid.mouse_buttons = 0U;
                g_hid.key_count = 0U;
                (void)memset(g_hid.keyboard_keys, 0, sizeof(g_hid.keyboard_keys));
                osal_printk("[svc_usb_hid] gamepad buttons=0x%08x hat=%u lx=%d ly=%d rx=%d ry=%d.\r\n",
                    (unsigned int)pkt.buttons, (unsigned int)pkt.hat,
                    (int)pkt.x, (int)pkt.y, (int)pkt.rx, (int)pkt.ry);
            } else if (g_hid.link_fail_count < 0xFFU) {
                g_hid.link_fail_count++;
                osal_printk("[svc_usb_hid] gamepad report send failed, buttons=0x%08x hat=%u.\r\n",
                    (unsigned int)pkt.buttons, (unsigned int)pkt.hat);
            }
        }
    } else if ((dx != 0) || (dy != 0) || (mouse_buttons != g_hid.mouse_buttons)) {
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
            (void)memset(&g_hid.gamepad_report, 0, sizeof(g_hid.gamepad_report));
            osal_printk("[svc_usb_hid] mouse dx=%d dy=%d buttons=0x%02x pos_valid=%u.\r\n",
                (int)hid_clamp_i8(dx), (int)hid_clamp_i8(dy),
                (unsigned int)mouse_buttons, (unsigned int)pos.valid);
        } else if (g_hid.link_fail_count < 0xFFU) {
            g_hid.link_fail_count++;
            osal_printk("[svc_usb_hid] mouse report send failed, dx=%d dy=%d buttons=0x%02x.\r\n",
                (int)hid_clamp_i8(dx), (int)hid_clamp_i8(dy), (unsigned int)mouse_buttons);
        }
    }

    if (!wireless_gamepad_mode &&
        ((keyboard_count != g_hid.key_count) ||
        (memcmp(keyboard_keys, g_hid.keyboard_keys, sizeof(keyboard_keys)) != 0))) {
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
        hid_reset_cached_reports();
        hid_reset_motion_history();
        osal_printk("[svc_usb_hid] HID link lost, pause reports and wait re-probe.\r\n");
    }

    g_hid.prev_keys = keys_mask;
}
