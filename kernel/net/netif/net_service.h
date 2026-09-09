#ifndef KERNEL_NET_SERVICE_H
#define KERNEL_NET_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t dropped_packets;
    uint64_t tcp_retransmissions;
    uint32_t active_tcp_connections;
    uint32_t active_sockets;
    uint64_t net_errors;
} NetStats;

void net_service_init(void);
void net_service_poll(void);
bool net_poll(void);
const NetStats* net_service_get_stats(void);

#endif // KERNEL_NET_SERVICE_H
