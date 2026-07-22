#ifndef KERNEL_NET_SERVICE_H
#define KERNEL_NET_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

void net_service_init(void);
void net_service_poll(void);

#endif // KERNEL_NET_SERVICE_H
