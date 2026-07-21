#ifndef SIGNATURES_IPV4_H
#define SIGNATURES_IPV4_H

#include <stdint.h>
#include <stdbool.h>

#define IP_VERSION_4        4
#define IP_MIN_IHL          5
#define IP_MIN_HLEN         20
#define IP_DEFAULT_TTL      64

#define IP_PROTO_ICMP       1
#define IP_PROTO_TCP        6
#define IP_PROTO_UDP        17

typedef enum {
    ROUTE_TYPE_LOCAL = 0,
    ROUTE_TYPE_GATEWAY
} RouteType;

struct ip_hdr {
    uint8_t  ihl_ver;      // Version (4 bits) | IHL (4 bits)
    uint8_t  tos;          // Type of service / DSCP
    uint16_t total_len;    // Total packet length (Big Endian)
    uint16_t id;           // Identification (Big Endian)
    uint16_t frag_off;     // Flags (3 bits) + Fragment offset (13 bits)
    uint8_t  ttl;          // Time to live
    uint8_t  proto;        // Protocol (1=ICMP, 6=TCP, 17=UDP)
    uint16_t checksum;     // Header Checksum (Big Endian)
    uint32_t src_ip;       // Source IP
    uint32_t dest_ip;      // Destination IP
} __attribute__((packed));

void ipv4_init(void);
uint16_t net_checksum(const void* buf, uint16_t len);
RouteType ipv4_route(uint32_t dest_ip, uint32_t* next_hop_ip);

bool ipv4_send(uint32_t dest_ip, uint8_t protocol, const void* payload, uint16_t payload_len);
void ipv4_process_packet(const uint8_t* payload, uint16_t length);

#endif // SIGNATURES_IPV4_H
