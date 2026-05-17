#ifndef OF_SM_H
#define OF_SM_H

#include "of_event.h"
#include "of_types.h"

void of_sm_init(void);
void of_sm_step(void);
void of_sm_post_event(of_event_t evt);
of_boot_state_t of_sm_get_boot_state(void);
of_mode_t of_sm_get_mode(void);

#endif
