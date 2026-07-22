#include "net_service.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/net/tcp/tcp.h"
#include "arch/x86_64/io/port_io.h"

extern void tcp_reclaim_stale_connections(void);
extern void tcp_check_half_open_timeouts(void);

static NetStats g_net_stats = {0};

void net_service_init(void) {
    // Network subsystem initialized
}

void net_service_poll(void) {
    E1000Frame frame;
    if (e1000_poll_receive(&frame)) {
        g_net_stats.rx_packets++;
        g_net_stats.rx_bytes += frame.length;
        ethernet_process_frame(frame.data, frame.length);
    }
    tcp_reclaim_stale_connections();
    tcp_check_half_open_timeouts();
}

const NetStats* net_service_get_stats(void) {
    return &g_net_stats;
}
