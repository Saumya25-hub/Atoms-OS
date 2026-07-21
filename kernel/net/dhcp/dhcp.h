#ifndef SIGNATURES_DHCP_H
#define SIGNATURES_DHCP_H

#include <stdint.h>
#include <stdbool.h>

#define DHCP_CLIENT_PORT    68
#define DHCP_SERVER_PORT    67

#define DHCP_MAGIC_COOKIE   0x63825363

#define DHCP_OP_BOOTREQUEST 1
#define DHCP_OP_BOOTREPLY   2

#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET  6

#define DHCP_OPTION_SUBNET_MASK     1
#define DHCP_OPTION_ROUTER          3
#define DHCP_OPTION_DNS_SERVER      6
#define DHCP_OPTION_REQ_IP          50
#define DHCP_OPTION_LEASE_TIME      51
#define DHCP_OPTION_MSG_TYPE        53
#define DHCP_OPTION_SERVER_ID       54
#define DHCP_OPTION_T1              58
#define DHCP_OPTION_T2              59
#define DHCP_OPTION_END             255

#define DHCP_MSG_DISCOVER           1
#define DHCP_MSG_OFFER              2
#define DHCP_MSG_REQUEST            3
#define DHCP_MSG_DECLINE            4
#define DHCP_MSG_ACK                5
#define DHCP_MSG_NAK                6

typedef enum {
    DHCP_STATE_INIT = 0,
    DHCP_STATE_SELECTING,
    DHCP_STATE_REQUESTING,
    DHCP_STATE_BOUND,
    DHCP_STATE_RENEWING,
    DHCP_STATE_REBINDING,
    DHCP_STATE_FAILED
} DhcpState;

struct dhcp_packet {
    uint8_t  op;           // 1 = BOOTREQUEST, 2 = BOOTREPLY
    uint8_t  htype;        // 1 = Ethernet
    uint8_t  hlen;         // 6
    uint8_t  hops;         // 0
    uint32_t xid;          // Transaction ID (Big Endian)
    uint16_t secs;         // Seconds elapsed
    uint16_t flags;        // 0x8000 = Broadcast
    uint32_t ciaddr;       // Client IP
    uint32_t yiaddr;       // 'Your' (offered) IP
    uint32_t siaddr;       // Server IP
    uint32_t giaddr;       // Gateway IP
    uint8_t  chaddr[16];   // Client MAC address
    char     sname[64];    // Server host name
    char     file[128];    // Boot file name
    uint32_t magic;        // Magic Cookie (0x63825363 in Network Byte Order)
    uint8_t  options[308]; // Options payload
} __attribute__((packed));

typedef struct {
    uint32_t  offered_ip;
    uint32_t  subnet_mask;
    uint32_t  gateway;
    uint32_t  dns_server;
    uint32_t  server_id;
    uint32_t  lease_time;
    uint32_t  t1_time;
    uint32_t  t2_time;
    uint32_t  xid;
    DhcpState state;
    bool      offer_received;
    bool      ack_received;
} DhcpLease;

void dhcp_init(void);
DhcpState dhcp_get_state(void);
const DhcpLease* dhcp_get_lease(void);

bool dhcp_start(void);
bool dhcp_run_dora(void);

#endif // SIGNATURES_DHCP_H
