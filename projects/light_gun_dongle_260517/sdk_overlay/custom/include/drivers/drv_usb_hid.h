#ifndef DRV_USB_HID_H
#define DRV_USB_HID_H

#include <stdint.h>

int drv_usb_hid_init(void);
void drv_usb_hid_deinit(void);
int drv_usb_hid_is_ready(void);
void drv_usb_hid_set_ready(int ready);
int drv_usb_hid_probe_ready(void);
int drv_usb_hid_release_all(void);
int drv_usb_hid_send_mouse_report(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel);
int drv_usb_hid_send_keyboard_report(const uint8_t *keys, uint8_t key_count);
int drv_usb_hid_send_gamepad_report(const void *report, uint32_t report_len);

#endif
