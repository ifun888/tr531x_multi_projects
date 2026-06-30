#ifndef DRV_INPUT_KEYS_H
#define DRV_INPUT_KEYS_H

#include "of_fops.h"
#include <stdint.h>

#define OF_KEY_MASK_TRIGGER   (1U << 0)
#define OF_KEY_MASK_A         (1U << 1)
#define OF_KEY_MASK_B         (1U << 2)
#define OF_KEY_MASK_START     (1U << 3)
#define OF_KEY_MASK_SELECT    (1U << 4)
#define OF_KEY_MASK_HOME      (1U << 5)
#define OF_KEY_MASK_UP        (1U << 6)
#define OF_KEY_MASK_DOWN      (1U << 7)
#define OF_KEY_MASK_LEFT      (1U << 8)
#define OF_KEY_MASK_RIGHT     (1U << 9)
#define OF_KEY_MASK_MIDDLE    (1U << 10)

const of_dev_t *drv_input_keys_get_dev(void);
int drv_input_keys_is_ready(void);
uint16_t drv_input_keys_get_cached_mask(void);

#endif
