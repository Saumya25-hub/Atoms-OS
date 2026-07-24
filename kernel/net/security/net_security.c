#include "net_security.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static ATOMS_FirewallRule g_rules[32];
static uint32_t           g_rule_count = 0;

void ATOMS_NetSecurity_Init(void) {
    g_rule_count = 0;
    bwe_log("INFO", "ATOMS Network Security Firewall Initialized");
}

bool ATOMS_NetSecurity_ValidatePacket(const void* packet_bytes, uint32_t size) {
    if (!packet_bytes || size < 14 || size > 1518) {
        return false; // Malformed size
    }
    return true;
}

bool ATOMS_NetSecurity_AddFirewallRule(uint32_t src_ip, uint16_t port, FirewallAction action) {
    if (g_rule_count < 32) {
        g_rules[g_rule_count].src_ip = src_ip;
        g_rules[g_rule_count].port = port;
        g_rules[g_rule_count].action = action;
        g_rule_count++;
        return true;
    }
    return false;
}
