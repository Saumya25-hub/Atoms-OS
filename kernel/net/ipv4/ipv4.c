#include "ipv4.h"
#include "kernel/net/netif.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/net/arp/arp.h"
#include "kernel/net/icmp/icmp.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint64_t val);

static uint16_t g_ip_id_counter = 1;

void ipv4_init(void) {
    g_ip_id_counter = 1;
}

uint16_t net_checksum(const void* buf, uint16_t len) {
    const uint8_t* bytes = (const uint8_t*)buf;
    uint32_t sum = 0;

    for (uint16_t i = 0; i < (len & ~1U); i += 2) {
        uint16_t word = ((uint16_t)bytes[i] << 8) | bytes[i + 1];
        sum += word;
    }

    if (len & 1) {
        sum += ((uint16_t)bytes[len - 1] << 8);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return htons((uint16_t)(~sum));
}

RouteType ipv4_route(uint32_t dest_ip, uint32_t* next_hop_ip) {
    NetInterface* netif = netif_get_default();
    if (!netif) {
        if (next_hop_ip) *next_hop_ip = dest_ip;
        return ROUTE_TYPE_LOCAL;
    }

    uint32_t local_net = ntohl(netif->ip_addr) & ntohl(netif->netmask);
    uint32_t dest_net = ntohl(dest_ip) & ntohl(netif->netmask);

    if (local_net == dest_net) {
        if (next_hop_ip) *next_hop_ip = dest_ip;
        return ROUTE_TYPE_LOCAL;
    } else {
        if (next_hop_ip) *next_hop_ip = netif->gateway_ip;
        return ROUTE_TYPE_GATEWAY;
    }
}

bool ipv4_send(uint32_t dest_ip, uint8_t protocol, const void* payload, uint16_t payload_len) {
    if (payload_len > 1480) {
        return false;
    }

    NetInterface* netif = netif_get_default();
    if (!netif || !netif->link_up) {
        return false;
    }

    uint8_t packet[1500];
    memset(packet, 0, sizeof(packet));

    struct ip_hdr* ip = (struct ip_hdr*)packet;
    ip->ihl_ver = (4 << 4) | 5; // Version 4, IHL 5 (20 bytes)
    ip->tos = 0;
    ip->total_len = htons(20 + payload_len);
    ip->id = htons(g_ip_id_counter++);
    ip->frag_off = htons(0x4000); // DF bit set
    ip->ttl = IP_DEFAULT_TTL;
    ip->proto = protocol;
    ip->checksum = 0;
    ip->src_ip = netif->ip_addr;
    ip->dest_ip = dest_ip;

    if (payload && payload_len > 0) {
        memcpy(packet + 20, payload, payload_len);
    }

    ip->checksum = net_checksum(ip, 20);

    display_print("[IPV4 HEX DUMP] ");
    for (int b = 0; b < 20; b++) {
        display_print_hex(packet[b]); display_print(" ");
    }
    display_print("\n");

    uint32_t next_hop_ip = 0;
    ipv4_route(dest_ip, &next_hop_ip);

    uint8_t next_hop_mac[6] = {0};
    if (!arp_resolve(next_hop_ip, next_hop_mac)) {
        return false;
    }

    return ethernet_send(next_hop_mac, ETH_TYPE_IPV4, packet, 20 + payload_len);
}

void ipv4_process_packet(const uint8_t* payload, uint16_t length) {
    if (!payload || length < IP_MIN_HLEN) {
        display_print("[IPV4 RX DROP] Payload null or len < 20\n");
        return;
    }

    const struct ip_hdr* ip = (const struct ip_hdr*)payload;
    uint8_t version = ip->ihl_ver >> 4;
    uint8_t ihl = ip->ihl_ver & 0x0F;
    uint16_t hlen = ihl * 4;

    if (version != IP_VERSION_4 || ihl < IP_MIN_IHL || hlen > length) {
        display_print("[IPV4 RX DROP] Bad version/IHL\n");
        return;
    }

    uint16_t total_len = ntohs(ip->total_len);
    if (total_len < hlen || total_len > length) {
        display_print("[IPV4 RX DROP] Bad total_len="); display_print_dec(total_len); display_print(" vs length="); display_print_dec(length); display_print("\n");
        return;
    }

    uint16_t ck = net_checksum(ip, hlen);
    if (ck != 0) {
        display_print("[IPV4 RX DROP] Bad IP Checksum 0x"); display_print_hex(ck); display_print("\n");
        return;
    }

    NetInterface* netif = netif_get_default();
    if (netif && ip->dest_ip != netif->ip_addr) {
        display_print("[IPV4 RX DROP] Dest IP mismatch\n");
        return;
    }

    const uint8_t* ip_payload = payload + hlen;
    uint16_t ip_payload_len = total_len - hlen;

    display_print("[IPV4 RX] Proto="); display_print_dec(ip->proto); display_print(" len="); display_print_dec(ip_payload_len); display_print("\n");

    if (ip->proto == IP_PROTO_ICMP) {
        icmp_process_packet(ip->src_ip, ip_payload, ip_payload_len);
    }
}
