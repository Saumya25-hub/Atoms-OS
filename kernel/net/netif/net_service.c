#include "net_service.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/net/tcp/tcp.h"
#include "arch/x86_64/io/port_io.h"

extern void tcp_reclaim_stale_connections(void);

void net_service_init(void) {
    // Network subsystem initialized
}

void net_service_poll(void) {
    E1000Frame frame;
    if (e1000_poll_receive(&frame)) {
        ethernet_process_frame(frame.data, frame.length);
    }
}
