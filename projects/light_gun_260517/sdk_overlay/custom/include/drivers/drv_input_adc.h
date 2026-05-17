#ifndef DRV_INPUT_ADC_H
#define DRV_INPUT_ADC_H

#include "of_fops.h"

const of_dev_t *drv_input_adc_get_dev(void);
int drv_input_adc_is_ready(void);

#endif
