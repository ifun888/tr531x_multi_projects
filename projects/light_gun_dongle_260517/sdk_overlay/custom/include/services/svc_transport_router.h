#ifndef SVC_TRANSPORT_ROUTER_H
#define SVC_TRANSPORT_ROUTER_H

#include "of_types.h"

int svc_transport_route_init(of_link_type_t link);
int svc_transport_route_auto(void);
int svc_transport_route_tick(void);

#endif
