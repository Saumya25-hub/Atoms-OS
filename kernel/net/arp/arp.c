#include "arp.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/net/netif.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/lib/include/string.h"

static ArpCacheEntry g_arp_cache[ARP_CACHE_CAPACITY] = {0};
static uint32_t g_cache_count = 0;
static const uint8_t g_broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void arp_init(void) {
    memset(g_arp_cache, 0, sizeof(g_arp_cache));
    g_cache_count = 0;
}

void arp_cache_flush(void) {
    memset(g_arp_cache, 0, sizeof(g_arp_cache));
    g_cache_count = 0;
}

uint32_t arp_cache_get_count(void) {
    return g_cache_count;
}

bool arp_cache_lookup(uint32_t ip, uint8_t mac_out[6]) {
    for (int i = 0; i < ARP_CACHE_CAPACITY; i++) {
        if (g_arp_cache[i].state == ARP_STATE_RESOLVED && g_arp_cache[i].ip == ip) {
            if (mac_out) {
                memcpy(mac_out, g_arp_cache[i].mac, 6);
            }
            return true;
        }
    }
    return false;
}

void arp_cache_mark_pending(uint32_t ip) {
    for (int i = 0; i < ARP_CACHE_CAPACITY; i++) {
        if (g_arp_cache[i].state != ARP_STATE_EMPTY && g_arp_cache[i].ip == ip) {
            return;
        }
    }

    for (int i = 0; i < ARP_CACHE_CAPACITY; i++) {
        if (g_arp_cache[i].state == ARP_STATE_EMPTY) {
            g_arp_cache[i].ip = ip;
            g_arp_cache[i].state = ARP_STATE_PENDING;
            memset(g_arp_cache[i].mac, 0, 6);
            return;
        }
    }
}

void arp_cache_insert(uint32_t ip, const uint8_t mac[6]) {
    if (!mac) return;

    // Reject all-zero or broadcast MAC
    if (mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0) return;
    if (mac[0] == 0xFF && mac[1] == 0xFF && mac[2] == 0xFF && mac[3] == 0xFF && mac[4] == 0xFF && mac[5] == 0xFF) return;

    // Check if updating existing entry
    for (int i = 0; i < ARP_CACHE_CAPACITY; i++) {
        if (g_arp_cache[i].state != ARP_STATE_EMPTY && g_arp_cache[i].ip == ip) {
            memcpy(g_arp_cache[i].mac, mac, 6);
            if (g_arp_cache[i].state != ARP_STATE_RESOLVED) {
                g_arp_cache[i].state = ARP_STATE_RESOLVED;
                g_cache_count++;
            }
            return;
        }
    }

    // Insert into first empty slot
    for (int i = 0; i < ARP_CACHE_CAPACITY; i++) {
        if (g_arp_cache[i].state == ARP_STATE_EMPTY) {
            g_arp_cache[i].ip = ip;
            memcpy(g_arp_cache[i].mac, mac, 6);
            g_arp_cache[i].state = ARP_STATE_RESOLVED;
            g_cache_count++;
            return;
        }
    }

    // Cache full: deterministic overwrite at index 0
    g_arp_cache[0].ip = ip;
    memcpy(g_arp_cache[0].mac, mac, 6);
    g_arp_cache[0].state = ARP_STATE_RESOLVED;
}

volatile uint64_t g_arp_tx_created = 0;

bool arp_request(uint32_t target_ip) {
    g_arp_tx_created++;
    NetInterface* netif = netif_get_default();
    if (!netif || !netif->link_up) return false;

    struct arp_hdr arp;
    arp.htype = htons(ARP_HTYPE_ETHERNET);
    arp.ptype = htons(ARP_PTYPE_IPV4);
    arp.hlen = ARP_HLEN_ETHERNET;
    arp.plen = ARP_PLEN_IPV4;
    arp.opcode = htons(ARP_OP_REQUEST);

    memcpy(arp.sender_mac, netif->mac_addr, 6);
    arp.sender_ip = netif->ip_addr;

    memset(arp.target_mac, 0, 6);
    arp.target_ip = target_ip;

    return ethernet_send(g_broadcast_mac, ETH_TYPE_ARP, &arp, sizeof(struct arp_hdr));
}

void arp_process_packet(const uint8_t* payload, uint16_t length, const uint8_t eth_src_mac[6]) {
    (void)eth_src_mac;
    if (!payload || length < sizeof(struct arp_hdr)) {
        return;
    }

    const struct arp_hdr* arp = (const struct arp_hdr*)payload;
    if (ntohs(arp->htype) != ARP_HTYPE_ETHERNET || ntohs(arp->ptype) != ARP_PTYPE_IPV4) {
        return;
    }
    if (arp->hlen != ARP_HLEN_ETHERNET || arp->plen != ARP_PLEN_IPV4) {
        return;
    }

    NetInterface* netif = netif_get_default();
    if (!netif) return;

    uint16_t opcode = ntohs(arp->opcode);

    if (opcode == ARP_OP_REQUEST) {
        if (arp->target_ip == netif->ip_addr) {
            // Update cache with requester details
            arp_cache_insert(arp->sender_ip, arp->sender_mac);

            // Construct and transmit unicast ARP Reply
            struct arp_hdr reply;
            reply.htype = htons(ARP_HTYPE_ETHERNET);
            reply.ptype = htons(ARP_PTYPE_IPV4);
            reply.hlen = ARP_HLEN_ETHERNET;
            reply.plen = ARP_PLEN_IPV4;
            reply.opcode = htons(ARP_OP_REPLY);

            memcpy(reply.sender_mac, netif->mac_addr, 6);
            reply.sender_ip = netif->ip_addr;

            memcpy(reply.target_mac, arp->sender_mac, 6);
            reply.target_ip = arp->sender_ip;

            ethernet_send(arp->sender_mac, ETH_TYPE_ARP, &reply, sizeof(struct arp_hdr));
        }
    } else if (opcode == ARP_OP_REPLY) {
        if (arp->target_ip == netif->ip_addr) {
            arp_cache_insert(arp->sender_ip, arp->sender_mac);
        }
    }
}

bool arp_resolve(uint32_t target_ip, uint8_t mac_out[6]) {
    if (arp_cache_lookup(target_ip, mac_out)) {
        return true;
    }

    for (int retry = 0; retry < 5; retry++) {
        arp_cache_mark_pending(target_ip);

        if (arp_request(target_ip)) {
            E1000Frame frame;
            for (volatile int poll = 0; poll < 1000000; poll++) {
                io_in8(0x80); // Force QEMU TCG I/O exit to yield to host SLIRP event loop
                if (e1000_poll_receive(&frame)) {
                    ethernet_process_frame(frame.data, frame.length);
                    if (arp_cache_lookup(target_ip, mac_out)) {
                        return true;
                    }
                }
            }
        }
    }

    return arp_cache_lookup(target_ip, mac_out);
}
