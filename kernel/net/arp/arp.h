#ifndef SIGNATURES_ARP_H
#define SIGNATURES_ARP_H

#include <stdint.h>
#include <stdbool.h>

#define ARP_HTYPE_ETHERNET  1
#define ARP_PTYPE_IPV4      0x0800
#define ARP_HLEN_ETHERNET   6
#define ARP_PLEN_IPV4       4

#define ARP_OP_REQUEST      1
#define ARP_OP_REPLY        2

#define ARP_CACHE_CAPACITY  32

typedef enum {
    ARP_STATE_EMPTY = 0,
    ARP_STATE_PENDING,
    ARP_STATE_RESOLVED
} ArpState;

typedef struct {
    uint32_t ip;
    uint8_t  mac[6];
    ArpState state;
    uint64_t timestamp;
} ArpCacheEntry;

struct arp_hdr {
    uint16_t htype;
    uint16_t ptype;
    uint8_t  hlen;
    uint8_t  plen;
    uint16_t opcode;
    uint8_t  sender_mac[6];
    uint32_t sender_ip;
    uint8_t  target_mac[6];
    uint32_t target_ip;
} __attribute__((packed));

void arp_init(void);
bool arp_request(uint32_t target_ip);
void arp_process_packet(const uint8_t* payload, uint16_t length, const uint8_t eth_src_mac[6]);

bool arp_cache_lookup(uint32_t ip, uint8_t mac_out[6]);
void arp_cache_insert(uint32_t ip, const uint8_t mac[6]);
void arp_cache_mark_pending(uint32_t ip);

bool arp_resolve(uint32_t target_ip, uint8_t mac_out[6]);

uint32_t arp_cache_get_count(void);

#endif // SIGNATURES_ARP_H
