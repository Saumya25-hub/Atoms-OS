#ifndef SIGNATURES_ICMP_H
#define SIGNATURES_ICMP_H

#include <stdint.h>
#include <stdbool.h>

#define ICMP_TYPE_ECHO_REPLY    0
#define ICMP_TYPE_ECHO_REQUEST  8
#define ICMP_CODE_ECHO          0

struct icmp_hdr {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} __attribute__((packed));

typedef struct {
    uint32_t target_ip;
    uint32_t src_ip;
    uint16_t id;
    uint16_t seq;
    uint16_t req_total_len;
    uint16_t req_checksum;
    uint16_t rx_desc_idx;
    uint16_t rx_frame_len;
    uint16_t rx_ethertype;
    uint8_t  rx_src_mac[6];
    uint16_t rx_icmp_checksum;
    bool     ip_checksum_pass;
    bool     icmp_checksum_pass;
    bool     reply_received;
} IcmpPingResult;

void icmp_init(void);
bool icmp_send_ping(uint32_t target_ip, uint16_t id, uint16_t seq, const char* payload_str, uint16_t payload_len);
void icmp_process_packet(uint32_t src_ip, const uint8_t* payload, uint16_t length);

bool icmp_ping_target(uint32_t target_ip, uint16_t id, uint16_t seq, IcmpPingResult* out_result);

#endif // SIGNATURES_ICMP_H
