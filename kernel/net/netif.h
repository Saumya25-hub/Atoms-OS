#ifndef SIGNATURES_NETIF_H
#define SIGNATURES_NETIF_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    NETIF_STATE_UNCONFIGURED = 0,
    NETIF_STATE_CONFIGURING,
    NETIF_STATE_CONFIGURED,
    NETIF_STATE_FAILED
} NetifState;

typedef struct {
    uint8_t    mac_addr[6];
    uint32_t   ip_addr;        // Network byte order
    uint32_t   netmask;        // Network byte order
    uint32_t   gateway_ip;     // Network byte order
    uint32_t   dns_server;     // Network byte order
    uint32_t   dhcp_server;    // Network byte order
    uint32_t   lease_time;     // Host byte order (seconds)
    uint32_t   t1_time;        // Host byte order (seconds)
    uint32_t   t2_time;        // Host byte order (seconds)
    NetifState state;
    bool       link_up;
} NetInterface;

void netif_init(void);
NetInterface* netif_get_default(void);

void netif_set_config(uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns, uint32_t dhcp_server, uint32_t lease_time, uint32_t t1, uint32_t t2);
uint32_t netif_get_ip(void);
uint32_t netif_get_netmask(void);
uint32_t netif_get_gateway(void);
uint32_t netif_get_dns(void);
NetifState netif_get_state(void);

#endif // SIGNATURES_NETIF_H
