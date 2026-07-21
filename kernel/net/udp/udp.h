#ifndef SIGNATURES_UDP_H
#define SIGNATURES_UDP_H

#include <stdint.h>
#include <stdbool.h>

#define UDP_HLEN            8
#define MAX_UDP_HANDLERS    16

struct udp_hdr {
    uint16_t src_port;     // Big Endian
    uint16_t dest_port;    // Big Endian
    uint16_t length;       // Big Endian (Header + Payload)
    uint16_t checksum;     // Big Endian
} __attribute__((packed));

struct udp_pseudo_hdr {
    uint32_t src_ip;
    uint32_t dest_ip;
    uint8_t  zero;
    uint8_t  protocol;
    uint16_t udp_len;
} __attribute__((packed));

typedef void (*UdpPortHandler)(uint32_t src_ip, uint16_t src_port, const uint8_t* payload, uint16_t length);

void udp_init(void);
bool udp_register_handler(uint16_t port, UdpPortHandler handler);
bool udp_unregister_handler(uint16_t port);

bool udp_send(uint32_t src_ip, uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, const void* payload, uint16_t payload_len);
void udp_process_packet(uint32_t src_ip, uint32_t dest_ip, const uint8_t* payload, uint16_t length);

#endif // SIGNATURES_UDP_H
