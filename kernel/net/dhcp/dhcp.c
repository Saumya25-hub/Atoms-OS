#include "dhcp.h"
#include "kernel/net/udp/udp.h"
#include "kernel/net/netif.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint64_t val);

static DhcpLease g_lease = {0};
static const uint32_t DEFAULT_XID = 0x5349474E; // "SIGN"

static void dhcp_udp_callback(uint32_t src_ip, uint16_t src_port, const uint8_t* payload, uint16_t length);

void dhcp_init(void) {
    memset(&g_lease, 0, sizeof(DhcpLease));
    g_lease.xid = DEFAULT_XID;
    g_lease.state = DHCP_STATE_INIT;

    udp_register_handler(DHCP_CLIENT_PORT, dhcp_udp_callback);
}

DhcpState dhcp_get_state(void) {
    return g_lease.state;
}

const DhcpLease* dhcp_get_lease(void) {
    return &g_lease;
}

static void dhcp_parse_options(const uint8_t* opt_buf, uint16_t opt_len, uint8_t* msg_type, uint32_t* subnet, uint32_t* gw, uint32_t* dns, uint32_t* server_id, uint32_t* lease, uint32_t* t1, uint32_t* t2) {
    uint16_t offset = 0;
    while (offset < opt_len) {
        uint8_t code = opt_buf[offset];
        if (code == DHCP_OPTION_END) {
            break;
        }
        if (code == 0) { // PAD
            offset++;
            continue;
        }

        if (offset + 1 >= opt_len) break;
        uint8_t len = opt_buf[offset + 1];
        if (offset + 2 + len > opt_len) break;

        const uint8_t* val = &opt_buf[offset + 2];

        switch (code) {
            case DHCP_OPTION_MSG_TYPE:
                if (len >= 1 && msg_type) *msg_type = val[0];
                break;
            case DHCP_OPTION_SUBNET_MASK:
                if (len >= 4 && subnet) memcpy(subnet, val, 4);
                break;
            case DHCP_OPTION_ROUTER:
                if (len >= 4 && gw) memcpy(gw, val, 4);
                break;
            case DHCP_OPTION_DNS_SERVER:
                if (len >= 4 && dns) memcpy(dns, val, 4);
                break;
            case DHCP_OPTION_SERVER_ID:
                if (len >= 4 && server_id) memcpy(server_id, val, 4);
                break;
            case DHCP_OPTION_LEASE_TIME:
                if (len >= 4 && lease) {
                    uint32_t raw;
                    memcpy(&raw, val, 4);
                    *lease = ntohl(raw);
                }
                break;
            case DHCP_OPTION_T1:
                if (len >= 4 && t1) {
                    uint32_t raw;
                    memcpy(&raw, val, 4);
                    *t1 = ntohl(raw);
                }
                break;
            case DHCP_OPTION_T2:
                if (len >= 4 && t2) {
                    uint32_t raw;
                    memcpy(&raw, val, 4);
                    *t2 = ntohl(raw);
                }
                break;
            default:
                // Safely skip unknown options
                break;
        }

        offset += 2 + len;
    }
}

static bool mac_equals(const uint8_t m1[6], const uint8_t m2[6]) {
    for (int i = 0; i < 6; i++) {
        if (m1[i] != m2[i]) return false;
    }
    return true;
}

static void dhcp_udp_callback(uint32_t src_ip, uint16_t src_port, const uint8_t* payload, uint16_t length) {
    (void)src_ip;
    (void)src_port;

    if (!payload || length < (sizeof(struct dhcp_packet) - 308)) {
        return;
    }

    const struct dhcp_packet* pkt = (const struct dhcp_packet*)payload;
    if (pkt->op != DHCP_OP_BOOTREPLY || ntohl(pkt->xid) != g_lease.xid) {
        return;
    }

    NetInterface* netif = netif_get_default();
    if (netif) {
        if (!mac_equals(pkt->chaddr, netif->mac_addr)) {
            return;
        }
    }

    if (ntohl(pkt->magic) != DHCP_MAGIC_COOKIE) {
        return;
    }

    uint8_t msg_type = 0;
    uint32_t subnet = 0;
    uint32_t gw = 0;
    uint32_t dns = 0;
    uint32_t server_id = 0;
    uint32_t lease = 0;
    uint32_t t1 = 0;
    uint32_t t2 = 0;

    dhcp_parse_options(pkt->options, sizeof(pkt->options), &msg_type, &subnet, &gw, &dns, &server_id, &lease, &t1, &t2);

    if (msg_type == DHCP_MSG_OFFER) {
        g_lease.offered_ip = pkt->yiaddr;
        g_lease.subnet_mask = subnet;
        g_lease.gateway = gw;
        g_lease.dns_server = dns;
        g_lease.server_id = server_id;
        g_lease.lease_time = lease;
        g_lease.t1_time = (t1 > 0) ? t1 : (lease / 2);
        g_lease.t2_time = (t2 > 0) ? t2 : ((lease * 7) / 8);
        g_lease.offer_received = true;
        g_lease.state = DHCP_STATE_REQUESTING;
    } else if (msg_type == DHCP_MSG_ACK) {
        g_lease.offered_ip = pkt->yiaddr;
        if (subnet) g_lease.subnet_mask = subnet;
        if (gw) g_lease.gateway = gw;
        if (dns) g_lease.dns_server = dns;
        if (server_id) g_lease.server_id = server_id;
        if (lease) g_lease.lease_time = lease;
        g_lease.t1_time = (t1 > 0) ? t1 : (g_lease.lease_time / 2);
        g_lease.t2_time = (t2 > 0) ? t2 : ((g_lease.lease_time * 7) / 8);
        g_lease.ack_received = true;
        g_lease.state = DHCP_STATE_BOUND;

        // Atomically commit configuration to NetInterface
        netif_set_config(g_lease.offered_ip, g_lease.subnet_mask, g_lease.gateway, g_lease.dns_server, g_lease.server_id, g_lease.lease_time, g_lease.t1_time, g_lease.t2_time);
    }
}

static bool dhcp_send_discover(void) {
    NetInterface* netif = netif_get_default();
    if (!netif) return false;

    struct dhcp_packet pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.op = DHCP_OP_BOOTREQUEST;
    pkt.htype = DHCP_HTYPE_ETHERNET;
    pkt.hlen = DHCP_HLEN_ETHERNET;
    pkt.hops = 0;
    pkt.xid = htonl(g_lease.xid);
    pkt.secs = 0;
    pkt.flags = htons(0x8000); // Broadcast Flag
    pkt.ciaddr = 0;
    pkt.yiaddr = 0;
    pkt.siaddr = 0;
    pkt.giaddr = 0;
    memcpy(pkt.chaddr, netif->mac_addr, 6);
    pkt.magic = htonl(DHCP_MAGIC_COOKIE);

    uint16_t opt_idx = 0;
    // Option 53: DHCPDISCOVER
    pkt.options[opt_idx++] = DHCP_OPTION_MSG_TYPE;
    pkt.options[opt_idx++] = 1;
    pkt.options[opt_idx++] = DHCP_MSG_DISCOVER;

    // Option 55: Parameter Request List (Subnet Mask=1, Router=3, DNS=6)
    pkt.options[opt_idx++] = 55;
    pkt.options[opt_idx++] = 3;
    pkt.options[opt_idx++] = 1;
    pkt.options[opt_idx++] = 3;
    pkt.options[opt_idx++] = 6;

    // Option 255: END
    pkt.options[opt_idx++] = DHCP_OPTION_END;

    g_lease.state = DHCP_STATE_SELECTING;
    g_lease.offer_received = false;

    return udp_send(0, 0xFFFFFFFF, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, &pkt, sizeof(pkt));
}

static bool dhcp_send_request(uint32_t requested_ip, uint32_t server_id) {
    NetInterface* netif = netif_get_default();
    if (!netif) return false;

    struct dhcp_packet pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.op = DHCP_OP_BOOTREQUEST;
    pkt.htype = DHCP_HTYPE_ETHERNET;
    pkt.hlen = DHCP_HLEN_ETHERNET;
    pkt.hops = 0;
    pkt.xid = htonl(g_lease.xid);
    pkt.secs = 0;
    pkt.flags = htons(0x8000); // Broadcast Flag
    pkt.ciaddr = 0;
    pkt.yiaddr = 0;
    pkt.siaddr = 0;
    pkt.giaddr = 0;
    memcpy(pkt.chaddr, netif->mac_addr, 6);
    pkt.magic = htonl(DHCP_MAGIC_COOKIE);

    uint16_t opt_idx = 0;
    // Option 53: DHCPREQUEST
    pkt.options[opt_idx++] = DHCP_OPTION_MSG_TYPE;
    pkt.options[opt_idx++] = 1;
    pkt.options[opt_idx++] = DHCP_MSG_REQUEST;

    // Option 50: Requested IP
    pkt.options[opt_idx++] = DHCP_OPTION_REQ_IP;
    pkt.options[opt_idx++] = 4;
    memcpy(&pkt.options[opt_idx], &requested_ip, 4);
    opt_idx += 4;

    // Option 54: Server Identifier
    if (server_id != 0) {
        pkt.options[opt_idx++] = DHCP_OPTION_SERVER_ID;
        pkt.options[opt_idx++] = 4;
        memcpy(&pkt.options[opt_idx], &server_id, 4);
        opt_idx += 4;
    }

    // Option 55: Parameter Request List
    pkt.options[opt_idx++] = 55;
    pkt.options[opt_idx++] = 3;
    pkt.options[opt_idx++] = 1;
    pkt.options[opt_idx++] = 3;
    pkt.options[opt_idx++] = 6;

    // Option 255: END
    pkt.options[opt_idx++] = DHCP_OPTION_END;

    g_lease.state = DHCP_STATE_REQUESTING;
    g_lease.ack_received = false;

    return udp_send(0, 0xFFFFFFFF, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, &pkt, sizeof(pkt));
}

bool dhcp_run_dora(void) {
    dhcp_init();

    // 1. Send DHCPDISCOVER
    display_print("[DHCP DISCOVER]\n");
    display_print("State             = INIT -> SELECTING\n");
    display_print("Transaction ID    = 0x5349474E\n");
    display_print("Source IP         = 0.0.0.0\n");
    display_print("Destination IP    = 255.255.255.255\n");
    display_print("Source Port       = 68\n");
    display_print("Destination Port  = 67\n");

    bool sent_disc = dhcp_send_discover();
    display_print("TX DMA            = "); display_print(sent_disc ? "PASS\n\n" : "FAIL\n\n");
    if (!sent_disc) return false;

    // Poll for DHCPOFFER
    extern bool net_poll(void);
    for (volatile int poll = 0; poll < 50000000; poll++) {
        io_in8(0x80);
        if (net_poll()) {
            if (g_lease.offer_received) break;
        }
    }

    if (!g_lease.offer_received) {
        display_print("[DHCP OFFER] FAIL (Timeout)\n");
        g_lease.state = DHCP_STATE_FAILED;
        return false;
    }

    // 2. Send DHCPREQUEST
    display_print("[DHCP REQUEST]\n");
    display_print("Requested IP      = Offered Value\n");
    display_print("Server Identifier = Server Value\n");

    bool sent_req = dhcp_send_request(g_lease.offered_ip, g_lease.server_id);
    display_print("TX Result         = "); display_print(sent_req ? "PASS\n\n" : "FAIL\n\n");
    if (!sent_req) return false;

    // Poll for DHCPACK
    for (volatile int poll = 0; poll < 50000000; poll++) {
        io_in8(0x80);
        if (net_poll()) {
            if (g_lease.ack_received) break;
        }
    }

    if (!g_lease.ack_received) {
        display_print("[DHCP ACK] FAIL (Timeout)\n");
        g_lease.state = DHCP_STATE_FAILED;
        return false;
    }

    return (g_lease.state == DHCP_STATE_BOUND);
}
