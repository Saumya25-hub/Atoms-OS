#include "netif.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/core/lib/include/string.h"

static NetInterface g_default_netif = {0};

void netif_init(void) {
    E1000Device* dev = e1000_get_device();
    if (dev) {
        memcpy(g_default_netif.mac_addr, dev->mac_addr, 6);
        g_default_netif.link_up = (dev->state >= E1000_STATE_RX_READY);
    }

    // Static IP configuration for QEMU SLIRP user network
    // IP: 10.0.2.15 (0x0A00020F)
    g_default_netif.ip_addr = (10U) | (0U << 8) | (2U << 16) | (15U << 24);
    // Subnet: 255.255.255.0
    g_default_netif.netmask = (255U) | (255U << 8) | (255U << 16) | (0U << 24);
    // Gateway: 10.0.2.2
    g_default_netif.gateway_ip = (10U) | (0U << 8) | (2U << 16) | (2U << 24);
}

NetInterface* netif_get_default(void) {
    return &g_default_netif;
}
