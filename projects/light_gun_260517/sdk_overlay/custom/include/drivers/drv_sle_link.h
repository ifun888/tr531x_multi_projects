#ifndef DRV_SLE_LINK_H
#define DRV_SLE_LINK_H

#include "of_fops.h"

const of_dev_t *drv_sle_link_get_dev(void);
int drv_sle_link_is_ready(void);
void drv_sle_link_push_rx(const uint8_t *buf, uint32_t len);
void drv_sle_link_set_connected(int connected);

#endif
