#ifndef DRV_USB_CDC_H
#define DRV_USB_CDC_H

#include "of_fops.h"

const of_dev_t *drv_usb_cdc_get_dev(void);
int drv_usb_cdc_is_ready(void);
void drv_usb_cdc_push_rx(const uint8_t *buf, uint32_t len);
void drv_usb_cdc_on_resume(void);
void drv_usb_cdc_on_suspend(void);

#endif
