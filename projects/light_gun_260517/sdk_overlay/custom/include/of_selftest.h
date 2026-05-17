#ifndef OF_SELFTEST_H
#define OF_SELFTEST_H

#include <stdint.h>
#include "of_types.h"

void of_selftest_run(void);
of_selftest_result_t of_selftest_get(of_selftest_item_t item);
int of_selftest_all_critical_pass(void);
uint32_t of_selftest_bitmap(void);

#endif
