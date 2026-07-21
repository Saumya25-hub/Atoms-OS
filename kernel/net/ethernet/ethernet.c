#include "ethernet.h"
#include "kernel/net/netif.h"
#include "kernel/net/arp/arp.h"
#include "kernel/net/ipv4/ipv4.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint64_t val);

bool ethernet_send(const uint8_t dest_mac[6], uint16_t ethertype, const void* payload, uint16_t payload_len) {
    if (!dest_mac || (!payload && payload_len > 0)) {
        return false;
    }

    uint16_t frame_len = ETH_HLEN + payload_len;
    uint16_t padded_len = (frame_len < ETH_MIN_LEN) ? ETH_MIN_LEN : frame_len;
    if (padded_len > ETH_MAX_LEN) {
        return false;
    }

    uint8_t frame_buf[ETH_MAX_LEN];
    memset(frame_buf, 0, padded_len);

    struct eth_hdr* eth = (struct eth_hdr*)frame_buf;
    memcpy(eth->dest_mac, dest_mac, ETH_ALEN);
    
    NetInterface* netif = netif_get_default();
    if (netif) {
        memcpy(eth->src_mac, netif->mac_addr, ETH_ALEN);
    }

    eth->ethertype = htons(ethertype);

    if (payload && payload_len > 0) {
        memcpy(frame_buf + ETH_HLEN, payload, payload_len);
    }

    return e1000_transmit_raw(frame_buf, padded_len);
}

void ethernet_process_frame(const uint8_t* frame, uint16_t length) {
    if (!frame || length < ETH_HLEN) {
        return;
    }

    const struct eth_hdr* eth = (const struct eth_hdr*)frame;
    uint16_t ethertype = ntohs(eth->ethertype);
    const uint8_t* payload = frame + ETH_HLEN;
    uint16_t payload_len = length - ETH_HLEN;

    display_print("[ETH RX] EtherType="); display_print_hex(ethertype); display_print(" len="); display_print_dec(length); display_print("\n");

    if (ethertype == ETH_TYPE_ARP) {
        arp_process_packet(payload, payload_len, eth->src_mac);
    } else if (ethertype == ETH_TYPE_IPV4) {
        ipv4_process_packet(payload, payload_len);
    }
}
