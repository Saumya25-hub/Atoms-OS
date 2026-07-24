#ifndef ATOMS_NET_SECURITY_H
#define ATOMS_NET_SECURITY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Network Security & Firewall Engine (Phase 11)
// ============================================================

typedef enum {
    FIREWALL_ACTION_ALLOW = 0,
    FIREWALL_ACTION_DENY
} FirewallAction;

typedef struct {
    uint32_t       src_ip;
    uint32_t       dest_ip;
    uint16_t       port;
    uint8_t        protocol;
    FirewallAction action;
} ATOMS_FirewallRule;

void ATOMS_NetSecurity_Init(void);
bool ATOMS_NetSecurity_ValidatePacket(const void* packet_bytes, uint32_t size);
bool ATOMS_NetSecurity_AddFirewallRule(uint32_t src_ip, uint16_t port, FirewallAction action);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_NET_SECURITY_H
