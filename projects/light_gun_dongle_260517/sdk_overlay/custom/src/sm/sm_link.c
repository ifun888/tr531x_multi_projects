#include "of_sm.h"
void sm_link_on_usb(int up){ of_sm_post_event(up?EVT_USB_MOUNTED:EVT_USB_LOST); }
void sm_link_on_sle(int up){ of_sm_post_event(up?EVT_SLE_CONNECTED:EVT_SLE_LOST); }
