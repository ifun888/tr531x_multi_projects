#include "platform/of_time.h"
extern uint64_t uapi_tcxo_get_us(void) __attribute__((weak));
uint64_t of_time_us(void){return (uapi_tcxo_get_us!=0)?uapi_tcxo_get_us():0;}
