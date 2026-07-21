#ifndef SIGNATURES_ETHERNET_H
#define SIGNATURES_ETHERNET_H

#include <stdint.h>
#include <stdbool.h>

#define ETH_ALEN        6
#define ETH_HLEN        14
#define ETH_MIN_LEN     60
#define ETH_MAX_LEN     1518

#define ETH_TYPE_IPV4   0x0800
#define ETH_TYPE_ARP    0x0806

// Network Byte Order Swapping Helpers
static inline uint16_t htons(uint16_t val) {
    return (uint16_t)((val << 8) | (val >> 8));
}

static inline uint16_t ntohs(uint16_t val) {
    return htons(val);
}

static inline uint32_t htonl(uint32_t val) {
    return ((val & 0xFF000000U) >> 24) |
           ((val & 0x00FF0000U) >> 8)  |
           ((val & 0x0000FF00U) << 8)  |
           ((val & 0x000000FFU) << 24);
}

static inline uint32_t ntohl(uint32_t val) {
    return htonl(val);
}

struct eth_hdr {
    uint8_t  dest_mac[6];
    uint8_t  src_mac[6];
    uint16_t ethertype;
} __attribute__((packed));

bool ethernet_send(const uint8_t dest_mac[6], uint16_t ethertype, const void* payload, uint16_t payload_len);
void ethernet_process_frame(const uint8_t* frame, uint16_t length);

#endif // SIGNATURES_ETHERNET_H
