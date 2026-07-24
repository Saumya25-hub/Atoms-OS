#ifndef ATOMS_NETWORK_MANAGER_H
#define ATOMS_NETWORK_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Enterprise Network Subsystem (Phase 11)
// ============================================================

#define ATOMS_MAX_NET_INTERFACES 8
#define ATOMS_ETH_MTU            1500
#define ATOMS_MAC_LEN            6

typedef enum {
    NET_IF_TYPE_LOOPBACK = 0,
    NET_IF_TYPE_ETHERNET,
    NET_IF_TYPE_WIFI
} NetInterfaceType;

typedef struct {
    uint32_t         if_id;
    char             name[16];          // "eth0", "lo"
    NetInterfaceType type;
    uint8_t          mac[ATOMS_MAC_LEN];
    uint32_t         ip_addr;           // IPv4 Address (Network Byte Order)
    uint32_t         netmask;           // Subnet Mask
    uint32_t         gateway;           // Gateway Address
    uint32_t         dns_server;        // Primary DNS Server
    uint32_t         mtu;               // MTU (default 1500)
    bool             link_up;           // Link Status
    uint32_t         rx_packets;        // Packet Statistics
    uint32_t         tx_packets;
    uint32_t         rx_bytes;
    uint32_t         tx_bytes;
    uint32_t         rx_errors;
    uint32_t         tx_errors;
} ATOMS_NetInterface;

void                ATOMS_NetworkManager_Init(void);
ATOMS_NetInterface* ATOMS_Net_RegisterInterface(const char* name, NetInterfaceType type, const uint8_t* mac);
ATOMS_NetInterface* ATOMS_Net_GetDefaultInterface(void);
ATOMS_NetInterface* ATOMS_Net_GetByName(const char* name);
uint32_t            ATOMS_Net_GetInterfaceCount(void);
void                ATOMS_Net_DumpDiagnostics(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_NETWORK_MANAGER_H
