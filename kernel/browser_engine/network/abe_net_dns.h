#ifndef ABE_NET_DNS_H
#define ABE_NET_DNS_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_DNS_CACHE_CAPACITY 32
#define ABE_MAX_DNS_SERVERS 4

typedef struct {
    char hostname[256];
    uint32_t ip_addr;     // Network byte order
    uint32_t ttl_sec;     // Time To Live
    uint64_t timestamp;  // Insertion time
    bool in_use;
} ABE_DNSCacheEntry;

typedef struct {
    uint32_t dns_servers[ABE_MAX_DNS_SERVERS];
    uint32_t dns_server_count;
    ABE_DNSCacheEntry cache[ABE_DNS_CACHE_CAPACITY];
    uint32_t cache_entry_count;
    uint32_t resolve_timeout_ms;
    uint32_t max_retries;
} ABE_DNSResolver;

ABE_Error ABE_NetDNS_Init(void);
ABE_Error ABE_NetDNS_Shutdown(void);

ABE_Error ABE_NetDNS_Resolve(const char* hostname, uint32_t* out_ip);
ABE_Error ABE_NetDNS_CacheLookup(const char* hostname, uint32_t* out_ip);
void      ABE_NetDNS_CacheInsert(const char* hostname, uint32_t ip, uint32_t ttl);
void      ABE_NetDNS_FlushCache(void);

#ifdef __cplusplus
}
#endif

#endif // ABE_NET_DNS_H
