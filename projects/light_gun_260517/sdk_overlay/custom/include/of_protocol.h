#ifndef OF_PROTOCOL_H
#define OF_PROTOCOL_H

#include <stdint.h>
#include "of_types.h"

void of_proto_docked_process(const uint8_t *buf, uint32_t len);
void of_proto_mh_process(const uint8_t *buf, uint32_t len);
void of_proto_set_mode(of_mode_t mode);
of_mode_t of_proto_get_mode(void);
uint8_t of_proto_mh_input_mode(void);
uint8_t of_proto_mh_offscreen_mode(void);
int of_proto_mh_gamepad_enabled(void);
int of_proto_mh_gamepad_aim_with_left_stick(void);
int of_proto_mh_offscreen_button_enabled(void);

#endif
