#include "dns.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/net/udp/udp.h"
#include "kernel/net/netif.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/drivers/display/display.h"
#include "arch/x86_64/io/port_io.h"

// ----------------------------------------------------------------------------
// Static Globals & State
// ----------------------------------------------------------------------------
static DnsCacheEntry    g_dns_cache[DNS_CACHE_CAPACITY];
static uint32_t         g_dns_cache_count = 0;
static uint16_t         g_dns_xid_counter = 0x4000;
static DnsResolverState g_dns_state;

void dns_init(void) {
    memset(g_dns_cache, 0, sizeof(g_dns_cache));
    g_dns_cache_count = 0;
    g_dns_xid_counter = 0x4000;
    memset(&g_dns_state, 0, sizeof(g_dns_state));
}

uint32_t dns_cache_get_count(void) {
    return g_dns_cache_count;
}

const DnsResolverState* dns_get_state(void) {
    return &g_dns_state;
}

bool dns_cache_lookup(const char* hostname, uint32_t* out_ip) {
    if (!hostname || !out_ip) return false;
    for (uint32_t i = 0; i < g_dns_cache_count; i++) {
        if (g_dns_cache[i].in_use && strcmp(g_dns_cache[i].hostname, hostname) == 0) {
            *out_ip = g_dns_cache[i].ip_addr;
            return true;
        }
    }
    return false;
}

void dns_cache_insert(const char* hostname, uint32_t ip, uint32_t ttl) {
    if (!hostname || ip == 0) return;
    for (uint32_t i = 0; i < g_dns_cache_count; i++) {
        if (g_dns_cache[i].in_use && strcmp(g_dns_cache[i].hostname, hostname) == 0) {
            g_dns_cache[i].ip_addr = ip;
            g_dns_cache[i].ttl = ttl;
            return;
        }
    }
    if (g_dns_cache_count < DNS_CACHE_CAPACITY) {
        strncpy(g_dns_cache[g_dns_cache_count].hostname, hostname, sizeof(g_dns_cache[0].hostname) - 1);
        g_dns_cache[g_dns_cache_count].ip_addr = ip;
        g_dns_cache[g_dns_cache_count].ttl = ttl;
        g_dns_cache[g_dns_cache_count].in_use = true;
        g_dns_cache_count++;
    }
}

uint16_t dns_encode_name(const char* src_name, uint8_t* dst_buf, uint16_t max_len) {
    if (!src_name || !dst_buf || max_len == 0) return 0;
    uint16_t src_len = (uint16_t)strlen(src_name);
    if (src_len + 2 > max_len) return 0;

    uint16_t dst_idx = 0;
    uint16_t label_start = 0;

    for (uint16_t i = 0; i <= src_len; i++) {
        if (src_name[i] == '.' || src_name[i] == '\0') {
            uint8_t label_len = (uint8_t)(i - label_start);
            if (label_len > 63) return 0;
            dst_buf[dst_idx++] = label_len;
            for (uint8_t j = 0; j < label_len; j++) {
                dst_buf[dst_idx++] = (uint8_t)src_name[label_start + j];
            }
            label_start = i + 1;
        }
    }
    dst_buf[dst_idx++] = 0;
    return dst_idx;
}

uint16_t dns_decode_name(const uint8_t* pkt_start, uint16_t offset, uint16_t max_pkt_len, char* out_name, uint16_t out_max) {
    if (!pkt_start || !out_name || out_max == 0 || offset >= max_pkt_len) return 0;
    uint16_t curr = offset;
    uint16_t out_idx = 0;
    bool jumped = false;
    uint16_t jump_bytes_consumed = 0;
    uint32_t safety_limit = 0;

    while (curr < max_pkt_len && safety_limit++ < 256) {
        uint8_t len = pkt_start[curr];
        if (len == 0) {
            if (!jumped) jump_bytes_consumed = (curr + 1) - offset;
            break;
        }
        if ((len & 0xC0) == 0xC0) {
            if (curr + 1 >= max_pkt_len) return 0;
            uint16_t ptr_offset = ((len & 0x3F) << 8) | pkt_start[curr + 1];
            if (!jumped) {
                jump_bytes_consumed = (curr + 2) - offset;
                jumped = true;
            }
            curr = ptr_offset;
            continue;
        }
        curr++;
        if (curr + len > max_pkt_len) return 0;
        if (out_idx > 0 && out_idx < out_max - 1) {
            out_name[out_idx++] = '.';
        }
        for (uint8_t i = 0; i < len; i++) {
            if (out_idx < out_max - 1) {
                out_name[out_idx++] = (char)pkt_start[curr + i];
            }
        }
        curr += len;
    }
    out_name[out_idx] = '\0';
    return jumped ? jump_bytes_consumed : (curr + 1 - offset);
}

void dns_udp_callback(uint32_t src_ip, uint16_t src_port, const uint8_t* payload, uint16_t length) {
    (void)src_ip; (void)src_port;
    if (!payload || length < sizeof(struct dns_hdr)) return;
    struct dns_hdr* hdr = (struct dns_hdr*)payload;
    if (ntohs(hdr->id) != g_dns_state.tx_id) return;
    uint16_t flags = ntohs(hdr->flags);
    if (!(flags & DNS_FLAG_QR)) return;
    uint8_t rcode = (uint8_t)(flags & 0x000F);
    if (rcode != 0) { // 0 = DNS_RCODE_NOERROR
        g_dns_state.response_received = true;
        g_dns_state.match_success = false;
        return;
    }
    uint16_t qdcount = ntohs(hdr->qdcount);
    uint16_t ancount = ntohs(hdr->ancount);
    uint16_t offset = sizeof(struct dns_hdr);

    for (uint16_t q = 0; q < qdcount; q++) {
        char qname[256];
        uint16_t consumed = dns_decode_name(payload, offset, length, qname, sizeof(qname));
        if (consumed == 0) return;
        offset += consumed + 4;
        if (offset > length) return;
    }

    for (uint16_t a = 0; a < ancount; a++) {
        char aname[256];
        uint16_t consumed = dns_decode_name(payload, offset, length, aname, sizeof(aname));
        if (consumed == 0) return;
        offset += consumed;
        if (offset + 10 > length) return;

        uint16_t atype = (payload[offset] << 8) | payload[offset + 1];
        uint32_t ttl   = (payload[offset + 4] << 24) | (payload[offset + 5] << 16) | (payload[offset + 6] << 8) | payload[offset + 7];
        uint16_t rdlength = (payload[offset + 8] << 8) | payload[offset + 9];
        offset += 10;

        if (atype == DNS_TYPE_A && rdlength == 4 && offset + 4 <= length) {
            uint32_t ip = (payload[offset + 3] << 24) | (payload[offset + 2] << 16) | (payload[offset + 1] << 8) | payload[offset];
            g_dns_state.resolved_ip = ip;
            g_dns_state.resolved_ttl = ttl;
            strncpy(g_dns_state.resolved_name, aname, sizeof(g_dns_state.resolved_name) - 1);
            g_dns_state.response_received = true;
            g_dns_state.match_success = true;
            return;
        }
        offset += rdlength;
    }
}

static void format_ip_str(uint32_t ip, char* buf) {
    uint8_t* b = (uint8_t*)&ip;
    char tmp[32];
    int idx = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t val = b[i];
        if (val >= 100) {
            tmp[idx++] = '0' + (val / 100); val %= 100;
            tmp[idx++] = '0' + (val / 10); val %= 10;
            tmp[idx++] = '0' + val;
        } else if (val >= 10) {
            tmp[idx++] = '0' + (val / 10); val %= 10;
            tmp[idx++] = '0' + val;
        } else {
            tmp[idx++] = '0' + val;
        }
        if (i < 3) tmp[idx++] = '.';
    }
    tmp[idx] = '\0';
    strcpy(buf, tmp);
}

bool dns_resolve_ipv4(const char* hostname, uint32_t* out_ip) {
    if (!hostname || !out_ip) return false;

    display_print("[MINBROW][NET] DNS_START host=");
    display_print(hostname);
    display_print("\n");

    // 1. Check DNS Cache first
    if (dns_cache_lookup(hostname, out_ip)) {
        char ip_str[32];
        format_ip_str(*out_ip, ip_str);
        display_print("[MINBROW][NET] DNS_RESULT ");
        display_print(ip_str);
        display_print(" (Cache Hit)\n");
        display_print("[MINBROW][NET] DNS_SUCCESS\n");
        return true;
    }

    NetInterface* netif = netif_get_default();
    if (!netif || netif->state != NETIF_STATE_CONFIGURED) {
        display_print("[MINBROW][NET] DNS_FAILURE reason=NETIF_NOT_CONFIGURED\n");
        return false;
    }

    uint32_t dns_servers[4];
    int server_count = 0;

    if (netif->dns_server != 0) {
        dns_servers[server_count++] = netif->dns_server;
    }
    dns_servers[server_count++] = 0x08080808; // 8.8.8.8 (Google DNS)
    dns_servers[server_count++] = 0x01010101; // 1.1.1.1 (Cloudflare DNS)
    dns_servers[server_count++] = 0x0302000A; // 10.0.2.3 (QEMU NAT Gateway)

    for (int s = 0; s < server_count; s++) {
        uint32_t dns_server_ip = dns_servers[s];
        char srv_str[32];
        format_ip_str(dns_server_ip, srv_str);

        display_print("[MINBROW][NET] DNS_SERVER ");
        display_print(srv_str);
        display_print("\n");

        memset(&g_dns_state, 0, sizeof(g_dns_state));
        g_dns_state.tx_id = g_dns_xid_counter++;
        g_dns_state.ephemeral_port = 49152 + (g_dns_state.tx_id % 16384);
        strncpy(g_dns_state.resolved_name, hostname, sizeof(g_dns_state.resolved_name) - 1);

        udp_register_handler(g_dns_state.ephemeral_port, dns_udp_callback);

        uint8_t pkt_buf[512];
        memset(pkt_buf, 0, sizeof(pkt_buf));

        struct dns_hdr* hdr = (struct dns_hdr*)pkt_buf;
        hdr->id = htons(g_dns_state.tx_id);
        hdr->flags = htons(DNS_FLAG_RD);
        hdr->qdcount = htons(1);

        uint16_t offset = sizeof(struct dns_hdr);
        uint16_t name_len = dns_encode_name(hostname, pkt_buf + offset, sizeof(pkt_buf) - offset);
        if (name_len == 0) {
            udp_unregister_handler(g_dns_state.ephemeral_port);
            continue;
        }

        offset += name_len;
        pkt_buf[offset++] = 0; pkt_buf[offset++] = DNS_TYPE_A;
        pkt_buf[offset++] = 0; pkt_buf[offset++] = DNS_CLASS_IN;

        bool sent = udp_send(netif->ip_addr, dns_server_ip, g_dns_state.ephemeral_port, DNS_PORT, pkt_buf, offset);
        if (!sent) {
            udp_unregister_handler(g_dns_state.ephemeral_port);
            continue;
        }

        display_print("[MINBROW][NET] DNS_QUERY_SENT\n");

        // Calibrated poll with timeout
        E1000Frame frame;
        for (volatile int poll = 0; poll < 1000000; poll++) {
            if (e1000_poll_receive(&frame)) {
                ethernet_process_frame(frame.data, frame.length);
                if (g_dns_state.response_received) break;
            }
        }

        udp_unregister_handler(g_dns_state.ephemeral_port);

        if (g_dns_state.match_success && g_dns_state.resolved_ip != 0) {
            *out_ip = g_dns_state.resolved_ip;
            dns_cache_insert(hostname, g_dns_state.resolved_ip, g_dns_state.resolved_ttl);

            char res_str[32];
            format_ip_str(*out_ip, res_str);

            display_print("[MINBROW][NET] DNS_RESPONSE_RECEIVED\n");
            display_print("[MINBROW][NET] DNS_RESULT ");
            display_print(res_str);
            display_print("\n");
            display_print("[MINBROW][NET] DNS_SUCCESS\n");
            return true;
        }
    }

    display_print("[MINBROW][NET] DNS_FAILURE reason=NO_RESPONSE_OR_UNRESOLVED\n");
    return false;
}
