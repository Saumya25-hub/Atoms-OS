#include "netif.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/net/arp/arp.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/core/lib/include/string.h"

#include "kernel/drivers/net/r8168/r8168.h"

static NetInterface g_default_netif = {0};

void netif_init(void) {
    R8168Device* rdev = r8168_get_device();
    E1000Device* dev = e1000_get_device();
    memset(&g_default_netif, 0, sizeof(NetInterface));

    if (rdev && rdev->state >= R8168_STATE_READY) {
        memcpy(g_default_netif.mac_addr, rdev->mac_addr, 6);
        g_default_netif.link_up = true;
    } else if (dev) {
        memcpy(g_default_netif.mac_addr, dev->mac_addr, 6);
        g_default_netif.link_up = (dev->state >= E1000_STATE_RX_READY);
    }
    g_default_netif.state = NETIF_STATE_UNCONFIGURED;
}

NetInterface* netif_get_default(void) {
    return &g_default_netif;
}

void netif_set_config(uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns, uint32_t dhcp_server, uint32_t lease_time, uint32_t t1, uint32_t t2) {
    g_default_netif.ip_addr = ip;
    g_default_netif.netmask = mask;
    g_default_netif.gateway_ip = gw;
    g_default_netif.dns_server = dns;
    g_default_netif.dhcp_server = dhcp_server;
    g_default_netif.lease_time = lease_time;
    g_default_netif.t1_time = t1;
    g_default_netif.t2_time = t2;
    g_default_netif.state = NETIF_STATE_CONFIGURED;

    // Reset ARP cache on identity change
    arp_cache_flush();
}

uint32_t netif_get_ip(void) {
    return g_default_netif.ip_addr;
}

uint32_t netif_get_netmask(void) {
    return g_default_netif.netmask;
}

uint32_t netif_get_gateway(void) {
    return g_default_netif.gateway_ip;
}

uint32_t netif_get_dns(void) {
    return g_default_netif.dns_server;
}

NetifState netif_get_state(void) {
    return g_default_netif.state;
}
