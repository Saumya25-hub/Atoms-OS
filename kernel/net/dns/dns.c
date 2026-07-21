#include "dns.h"
#include "kernel/net/udp/udp.h"
#include "kernel/net/netif.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/core/lib/include/string.h"
#include "arch/x86_64/io/port_io.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint64_t val);

static DnsCacheEntry g_dns_cache[DNS_CACHE_CAPACITY] = {0};
static DnsResolverState g_dns_state = {0};
static uint16_t g_dns_xid_counter = 0x5349;

void dns_init(void) {
    memset(g_dns_cache, 0, sizeof(g_dns_cache));
    memset(&g_dns_state, 0, sizeof(g_dns_state));
}

const DnsResolverState* dns_get_state(void) {
    return &g_dns_state;
}

uint32_t dns_cache_get_count(void) {
    uint32_t count = 0;
    for (int i = 0; i < DNS_CACHE_CAPACITY; i++) {
        if (g_dns_cache[i].in_use) count++;
    }
    return count;
}

bool dns_cache_lookup(const char* hostname, uint32_t* out_ip) {
    if (!hostname || !out_ip) return false;

    for (int i = 0; i < DNS_CACHE_CAPACITY; i++) {
        if (g_dns_cache[i].in_use && strcmp(g_dns_cache[i].hostname, hostname) == 0) {
            *out_ip = g_dns_cache[i].ip_addr;
            return true;
        }
    }
    return false;
}

void dns_cache_insert(const char* hostname, uint32_t ip, uint32_t ttl) {
    if (!hostname) return;

    // Check if already in cache
    for (int i = 0; i < DNS_CACHE_CAPACITY; i++) {
        if (g_dns_cache[i].in_use && strcmp(g_dns_cache[i].hostname, hostname) == 0) {
            g_dns_cache[i].ip_addr = ip;
            g_dns_cache[i].ttl = ttl;
            return;
        }
    }

    // Insert into first free entry
    for (int i = 0; i < DNS_CACHE_CAPACITY; i++) {
        if (!g_dns_cache[i].in_use) {
            strncpy(g_dns_cache[i].hostname, hostname, 63);
            g_dns_cache[i].hostname[63] = '\0';
            g_dns_cache[i].ip_addr = ip;
            g_dns_cache[i].ttl = ttl;
            g_dns_cache[i].in_use = true;
            return;
        }
    }

    // Cache full: deterministic replacement at slot 0
    strncpy(g_dns_cache[0].hostname, hostname, 63);
    g_dns_cache[0].hostname[63] = '\0';
    g_dns_cache[0].ip_addr = ip;
    g_dns_cache[0].ttl = ttl;
    g_dns_cache[0].in_use = true;
}

// Convert "www.google.com" -> "\x03www\x06google\x03com\x00"
static uint16_t dns_encode_name(const char* hostname, uint8_t* out_buf, uint16_t max_buf) {
    if (!hostname || !out_buf || max_buf < 2) return 0;

    uint16_t out_idx = 0;
    uint16_t label_len_idx = 0;
    uint8_t label_len = 0;

    label_len_idx = out_idx++;
    out_buf[label_len_idx] = 0;

    for (int i = 0; hostname[i] != '\0'; i++) {
        if (out_idx >= max_buf - 2) return 0;

        if (hostname[i] == '.') {
            out_buf[label_len_idx] = label_len;
            label_len = 0;
            label_len_idx = out_idx++;
            out_buf[label_len_idx] = 0;
        } else {
            out_buf[out_idx++] = (uint8_t)hostname[i];
            label_len++;
        }
    }

    out_buf[label_len_idx] = label_len;
    out_buf[out_idx++] = 0; // Terminating null label

    return out_idx;
}

// Safe DNS Name Decompression & Label Parser
static bool dns_parse_name(const uint8_t* pkt, uint16_t pkt_len, uint16_t offset, char* out_name, uint16_t max_name, uint16_t* bytes_consumed) {
    if (!pkt || offset >= pkt_len || !out_name || max_name == 0) return false;

    uint16_t curr = offset;
    uint16_t name_idx = 0;
    uint16_t jumps = 0;
    uint16_t initial_bytes = 0;
    bool jumped = false;

    while (curr < pkt_len && jumps < 16) {
        uint8_t len = pkt[curr];
        if (len == 0) {
            if (!jumped) initial_bytes += 1;
            curr++;
            break;
        }

        // Check for DNS Compression Pointer (0xC0 prefix)
        if ((len & 0xC0) == 0xC0) {
            if (curr + 1 >= pkt_len) return false;
            uint16_t ptr_offset = ((uint16_t)(len & 0x3F) << 8) | pkt[curr + 1];
            if (ptr_offset >= pkt_len) return false;

            if (!jumped) {
                initial_bytes += 2;
                jumped = true;
            }

            curr = ptr_offset;
            jumps++;
            continue;
        }

        // Standard Label
        if (curr + 1 + len > pkt_len) return false;
        if (!jumped) initial_bytes += (1 + len);

        if (name_idx > 0 && name_idx < max_name - 1) {
            out_name[name_idx++] = '.';
        }

        for (uint8_t i = 0; i < len && name_idx < max_name - 1; i++) {
            out_name[name_idx++] = (char)pkt[curr + 1 + i];
        }

        curr += (1 + len);
    }

    out_name[name_idx] = '\0';
    if (bytes_consumed) *bytes_consumed = initial_bytes;

    return (name_idx > 0);
}

static void dns_udp_callback(uint32_t src_ip, uint16_t src_port, const uint8_t* payload, uint16_t length) {
    (void)src_ip;

    if (src_port != DNS_PORT || !payload || length < sizeof(struct dns_hdr)) {
        return;
    }

    const struct dns_hdr* hdr = (const struct dns_hdr*)payload;
    uint16_t rx_id = ntohs(hdr->id);
    uint16_t flags = ntohs(hdr->flags);
    uint16_t qdcount = ntohs(hdr->qdcount);
    uint16_t ancount = ntohs(hdr->ancount);

    if (rx_id != g_dns_state.tx_id) {
        return;
    }

    // Verify Response Flag (QR=1) and NOERROR (RCODE=0)
    if (!(flags & DNS_FLAG_QR) || (flags & 0x000F) != 0 || ancount == 0) {
        g_dns_state.response_received = true;
        g_dns_state.match_success = false;
        return;
    }

    uint16_t offset = sizeof(struct dns_hdr);

    // Skip Question Section
    for (uint16_t q = 0; q < qdcount; q++) {
        char qname[128];
        uint16_t consumed = 0;
        if (!dns_parse_name(payload, length, offset, qname, sizeof(qname), &consumed)) {
            return;
        }
        offset += consumed + 4; // Skip QNAME + QTYPE (2) + QCLASS (2)
        if (offset > length) return;
    }

    // Parse Answer Section
    for (uint16_t a = 0; a < ancount; a++) {
        if (offset >= length) break;

        char aname[128];
        uint16_t consumed = 0;
        if (!dns_parse_name(payload, length, offset, aname, sizeof(aname), &consumed)) {
            return;
        }
        offset += consumed;

        if (offset + 10 > length) return;

        uint16_t type = (payload[offset] << 8) | payload[offset + 1];
        uint16_t class_code = (payload[offset + 2] << 8) | payload[offset + 3];
        uint32_t ttl = ((uint32_t)payload[offset + 4] << 24) | ((uint32_t)payload[offset + 5] << 16) |
                       ((uint32_t)payload[offset + 6] << 8) | (uint32_t)payload[offset + 7];
        uint16_t rdlength = (payload[offset + 8] << 8) | payload[offset + 9];

        offset += 10;

        if (offset + rdlength > length) return;

        if (type == DNS_TYPE_A && class_code == DNS_CLASS_IN && rdlength == 4) {
            uint32_t raw_ip;
            memcpy(&raw_ip, payload + offset, 4);

            g_dns_state.resolved_ip = raw_ip; // Network Byte Order
            g_dns_state.resolved_ttl = ttl;
            strncpy(g_dns_state.resolved_name, aname, sizeof(g_dns_state.resolved_name) - 1);
            g_dns_state.response_received = true;
            g_dns_state.match_success = true;
            return;
        }

        offset += rdlength;
    }
}

bool dns_resolve_ipv4(const char* hostname, uint32_t* out_ip) {
    if (!hostname || !out_ip) return false;

    // 1. Check DNS Cache first
    if (dns_cache_lookup(hostname, out_ip)) {
        return true;
    }

    // 2. Retrieve dynamic DNS Server from NetInterface
    NetInterface* netif = netif_get_default();
    if (!netif || netif->state != NETIF_STATE_CONFIGURED || netif->dns_server == 0) {
        return false;
    }

    uint32_t dns_server_ip = netif->dns_server;

    // 3. Prepare Resolution State
    memset(&g_dns_state, 0, sizeof(g_dns_state));
    g_dns_state.tx_id = g_dns_xid_counter++;
    g_dns_state.ephemeral_port = 49152 + (g_dns_state.tx_id % 16384);

    strncpy(g_dns_state.resolved_name, hostname, sizeof(g_dns_state.resolved_name) - 1);

    // 4. Register Ephemeral UDP Port Callback
    udp_register_handler(g_dns_state.ephemeral_port, dns_udp_callback);

    // 5. Construct DNS Query Packet
    uint8_t pkt_buf[512];
    memset(pkt_buf, 0, sizeof(pkt_buf));

    struct dns_hdr* hdr = (struct dns_hdr*)pkt_buf;
    hdr->id = htons(g_dns_state.tx_id);
    hdr->flags = htons(DNS_FLAG_RD); // Standard Query with Recursion Desired
    hdr->qdcount = htons(1);
    hdr->ancount = 0;
    hdr->nscount = 0;
    hdr->arcount = 0;

    uint16_t offset = sizeof(struct dns_hdr);
    uint16_t name_len = dns_encode_name(hostname, pkt_buf + offset, sizeof(pkt_buf) - offset);
    if (name_len == 0) {
        udp_unregister_handler(g_dns_state.ephemeral_port);
        return false;
    }

    offset += name_len;
    pkt_buf[offset++] = 0; pkt_buf[offset++] = DNS_TYPE_A;   // QTYPE: A (1)
    pkt_buf[offset++] = 0; pkt_buf[offset++] = DNS_CLASS_IN; // QCLASS: IN (1)

    // 6. Transmit Query over UDP Port 53
    bool sent = udp_send(netif->ip_addr, dns_server_ip, g_dns_state.ephemeral_port, DNS_PORT, pkt_buf, offset);
    if (!sent) {
        udp_unregister_handler(g_dns_state.ephemeral_port);
        return false;
    }

    // 7. Poll E1000 RX DMA Ring for Response
    E1000Frame frame;
    for (volatile int poll = 0; poll < 50000000; poll++) {
        io_in8(0x80);
        if (e1000_poll_receive(&frame)) {
            ethernet_process_frame(frame.data, frame.length);
            if (g_dns_state.response_received) break;
        }
    }

    udp_unregister_handler(g_dns_state.ephemeral_port);

    if (g_dns_state.match_success) {
        *out_ip = g_dns_state.resolved_ip;
        dns_cache_insert(hostname, g_dns_state.resolved_ip, g_dns_state.resolved_ttl);
        return true;
    }

    return false;
}
