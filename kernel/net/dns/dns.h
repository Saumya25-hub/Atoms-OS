#ifndef SIGNATURES_DNS_H
#define SIGNATURES_DNS_H

#include <stdint.h>
#include <stdbool.h>

#define DNS_PORT            53
#define DNS_TYPE_A          1
#define DNS_CLASS_IN        1
#define DNS_FLAG_RD         0x0100  // Recursion Desired
#define DNS_FLAG_QR         0x8000  // Response Flag
#define DNS_MAX_NAME_LEN    256
#define DNS_CACHE_CAPACITY  16

// 12-byte packed DNS Header (RFC 1035)
struct dns_hdr {
    uint16_t id;         // Transaction ID (Big Endian)
    uint16_t flags;      // Flags & Codes (Big Endian)
    uint16_t qdcount;    // Question Count (Big Endian)
    uint16_t ancount;    // Answer Record Count (Big Endian)
    uint16_t nscount;    // Authority Record Count (Big Endian)
    uint16_t arcount;    // Additional Record Count (Big Endian)
} __attribute__((packed));

typedef struct {
    char     hostname[64];
    uint32_t ip_addr;     // Network Byte Order
    uint32_t ttl;         // Seconds
    bool     in_use;
} DnsCacheEntry;

typedef struct {
    uint16_t tx_id;
    uint16_t ephemeral_port;
    uint32_t resolved_ip;
    uint32_t resolved_ttl;
    bool     response_received;
    bool     match_success;
    char     resolved_name[64];
} DnsResolverState;

void dns_init(void);
bool dns_resolve_ipv4(const char* hostname, uint32_t* out_ip);
bool dns_cache_lookup(const char* hostname, uint32_t* out_ip);
void dns_cache_insert(const char* hostname, uint32_t ip, uint32_t ttl);
uint32_t dns_cache_get_count(void);
const DnsResolverState* dns_get_state(void);

#endif // SIGNATURES_DNS_H
