#ifndef OF_DIAG_H
#define OF_DIAG_H

#include <stdint.h>

void of_diag_init(void);
void of_diag_on_tx(uint32_t bytes);
void of_diag_on_rx(uint32_t bytes);
void of_diag_on_rtt_us(uint32_t us);
void of_diag_tick(void);

#endif
