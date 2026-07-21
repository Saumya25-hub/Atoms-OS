#ifndef SIGNATURES_NETIF_H
#define SIGNATURES_NETIF_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t  mac_addr[6];
    uint32_t ip_addr;     // Host / Network byte order
    uint32_t netmask;
    uint32_t gateway_ip;
    bool     link_up;
} NetInterface;

void netif_init(void);
NetInterface* netif_get_default(void);

#endif // SIGNATURES_NETIF_H
