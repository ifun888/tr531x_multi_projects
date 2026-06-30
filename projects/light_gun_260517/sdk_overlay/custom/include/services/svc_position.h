#ifndef SVC_POSITION_H
#define SVC_POSITION_H

#include <stdint.h>

typedef enum {
    OF_POS_RUN_NORMAL = 0,
    OF_POS_RUN_AVERAGE,
    OF_POS_RUN_AVERAGE2,
} of_pos_run_mode_t;

typedef struct {
    uint16_t raw_x;
    uint16_t raw_y;
    uint16_t x;
    uint16_t y;
    uint8_t valid;
    uint8_t seen_count;
    uint8_t degraded;
    uint8_t run_mode;
} of_pos_sample_t;

void svc_position_init(void);
void svc_position_reset(void);
int svc_position_poll(void);
int svc_position_get(of_pos_sample_t *sample);
of_pos_run_mode_t svc_position_get_run_mode(void);

#endif
