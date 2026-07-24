#include "network_manager.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static ATOMS_NetInterface g_if_table[ATOMS_MAX_NET_INTERFACES];
static uint32_t           g_if_count = 0;

static void str_copy_limit(char* dest, const char* src, uint32_t limit) {
    if (!dest || !src || limit == 0) return;
    uint32_t i = 0;
    while (src[i] && i < limit - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void ATOMS_NetworkManager_Init(void) {
    g_if_count = 0;
    for (uint32_t i = 0; i < ATOMS_MAX_NET_INTERFACES; i++) {
        g_if_table[i].if_id = 0;
        g_if_table[i].link_up = false;
    }

    // Register Loopback interface lo (127.0.0.1)
    uint8_t lo_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    ATOMS_NetInterface* lo = ATOMS_Net_RegisterInterface("lo", NET_IF_TYPE_LOOPBACK, lo_mac);
    if (lo) {
        lo->ip_addr = 0x0100007F; // 127.0.0.1
        lo->netmask = 0x000000FF; // 255.0.0.0
        lo->link_up = true;
    }

    // Register primary Ethernet interface eth0 (E1000)
    uint8_t eth_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
    ATOMS_NetInterface* eth0 = ATOMS_Net_RegisterInterface("eth0", NET_IF_TYPE_ETHERNET, eth_mac);
    if (eth0) {
        eth0->ip_addr = 0x0F02000A; // 10.0.2.15 (QEMU default)
        eth0->netmask = 0x00FFFFFF; // 255.255.255.0
        eth0->gateway = 0x0202000A; // 10.0.2.2
        eth0->dns_server = 0x0302000A; // 10.0.2.3
        eth0->link_up = true;
    }

    bwe_log("INFO", "ATOMS Enterprise Network Subsystem Initialized");
}

ATOMS_NetInterface* ATOMS_Net_RegisterInterface(const char* name, NetInterfaceType type, const uint8_t* mac) {
    if (!name || g_if_count >= ATOMS_MAX_NET_INTERFACES) return 0;

    ATOMS_NetInterface* netif = &g_if_table[g_if_count];
    netif->if_id = g_if_count + 1;
    str_copy_limit(netif->name, name, sizeof(netif->name));
    netif->type = type;
    if (mac) {
        memcpy(netif->mac, mac, ATOMS_MAC_LEN);
    }
    netif->ip_addr = 0;
    netif->netmask = 0;
    netif->gateway = 0;
    netif->dns_server = 0;
    netif->mtu = ATOMS_ETH_MTU;
    netif->link_up = false;
    netif->rx_packets = 0;
    netif->tx_packets = 0;
    netif->rx_bytes = 0;
    netif->tx_bytes = 0;
    netif->rx_errors = 0;
    netif->tx_errors = 0;

    g_if_count++;
    return netif;
}

ATOMS_NetInterface* ATOMS_Net_GetDefaultInterface(void) {
    for (uint32_t i = 0; i < g_if_count; i++) {
        if (g_if_table[i].type == NET_IF_TYPE_ETHERNET && g_if_table[i].link_up) {
            return &g_if_table[i];
        }
    }
    return (g_if_count > 0) ? &g_if_table[0] : 0;
}

ATOMS_NetInterface* ATOMS_Net_GetByName(const char* name) {
    if (!name) return 0;
    for (uint32_t i = 0; i < g_if_count; i++) {
        if (strcmp(g_if_table[i].name, name) == 0) {
            return &g_if_table[i];
        }
    }
    return 0;
}

uint32_t ATOMS_Net_GetInterfaceCount(void) {
    return g_if_count;
}

void ATOMS_Net_DumpDiagnostics(void) {
    display_print("\n=======================================================\n");
    display_print("        ATOMS Enterprise Network Interface Status      \n");
    display_print("=======================================================\n");
    display_print("  Interface   Type       Status    MTU   RX Pkts  TX Pkts  \n");
    display_print("-------------------------------------------------------\n");

    for (uint32_t i = 0; i < g_if_count; i++) {
        display_print("  ");
        display_print(g_if_table[i].name);
        display_print("         ");
        display_print(g_if_table[i].type == NET_IF_TYPE_ETHERNET ? "Ethernet" : "Loopback");
        display_print("   ");
        display_print(g_if_table[i].link_up ? "UP  " : "DOWN");
        display_print("    ");
        display_print_dec(g_if_table[i].mtu);
        display_print("   ");
        display_print_dec(g_if_table[i].rx_packets);
        display_print("        ");
        display_print_dec(g_if_table[i].tx_packets);
        display_print("\n");
    }
    display_print("=======================================================\n\n");
}
