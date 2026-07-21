#include "udp.h"
#include "kernel/net/ipv4/ipv4.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint64_t val);

typedef struct {
    uint16_t       port;
    UdpPortHandler handler;
    bool           in_use;
} UdpHandlerEntry;

static UdpHandlerEntry g_udp_handlers[MAX_UDP_HANDLERS] = {0};

void udp_init(void) {
    memset(g_udp_handlers, 0, sizeof(g_udp_handlers));
}

bool udp_register_handler(uint16_t port, UdpPortHandler handler) {
    if (port == 0 || !handler) return false;

    // Check if already registered
    for (int i = 0; i < MAX_UDP_HANDLERS; i++) {
        if (g_udp_handlers[i].in_use && g_udp_handlers[i].port == port) {
            g_udp_handlers[i].handler = handler;
            return true;
        }
    }

    // Insert into first free slot
    for (int i = 0; i < MAX_UDP_HANDLERS; i++) {
        if (!g_udp_handlers[i].in_use) {
            g_udp_handlers[i].port = port;
            g_udp_handlers[i].handler = handler;
            g_udp_handlers[i].in_use = true;
            return true;
        }
    }

    return false;
}

bool udp_unregister_handler(uint16_t port) {
    for (int i = 0; i < MAX_UDP_HANDLERS; i++) {
        if (g_udp_handlers[i].in_use && g_udp_handlers[i].port == port) {
            g_udp_handlers[i].in_use = false;
            g_udp_handlers[i].port = 0;
            g_udp_handlers[i].handler = NULL;
            return true;
        }
    }
    return false;
}

// Calculate UDP checksum using IPv4 Pseudo-Header according to RFC 768 / RFC 1071
static uint16_t udp_calc_checksum(uint32_t src_ip, uint32_t dest_ip, const void* udp_data, uint16_t udp_len) {
    struct udp_pseudo_hdr pseudo;
    pseudo.src_ip = src_ip;
    pseudo.dest_ip = dest_ip;
    pseudo.zero = 0;
    pseudo.protocol = IP_PROTO_UDP;
    pseudo.udp_len = htons(udp_len);

    uint32_t sum = 0;
    const uint8_t* p_bytes = (const uint8_t*)&pseudo;
    for (uint16_t i = 0; i < sizeof(struct udp_pseudo_hdr); i += 2) {
        sum += ((uint16_t)p_bytes[i] << 8) | p_bytes[i + 1];
    }

    const uint8_t* u_bytes = (const uint8_t*)udp_data;
    for (uint16_t i = 0; i < (udp_len & ~1U); i += 2) {
        sum += ((uint16_t)u_bytes[i] << 8) | u_bytes[i + 1];
    }

    if (udp_len & 1) {
        sum += ((uint16_t)u_bytes[udp_len - 1] << 8);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    uint16_t ck = htons((uint16_t)(~sum));
    return (ck == 0) ? 0xFFFF : ck;
}

bool udp_send(uint32_t src_ip, uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, const void* payload, uint16_t payload_len) {
    if (payload_len > 1472) {
        return false;
    }

    uint16_t total_udp_len = UDP_HLEN + payload_len;
    uint8_t buf[1500];
    memset(buf, 0, sizeof(buf));

    struct udp_hdr* udp = (struct udp_hdr*)buf;
    udp->src_port = htons(src_port);
    udp->dest_port = htons(dest_port);
    udp->length = htons(total_udp_len);
    udp->checksum = 0;

    if (payload && payload_len > 0) {
        memcpy(buf + UDP_HLEN, payload, payload_len);
    }

    udp->checksum = udp_calc_checksum(src_ip, dest_ip, buf, total_udp_len);

    return ipv4_send(dest_ip, IP_PROTO_UDP, buf, total_udp_len);
}

void udp_process_packet(uint32_t src_ip, uint32_t dest_ip, const uint8_t* payload, uint16_t length) {
    if (!payload || length < UDP_HLEN) {
        display_print("[UDP RX DROP] Payload null or len < 8\n");
        return;
    }

    const struct udp_hdr* udp = (const struct udp_hdr*)payload;
    uint16_t src_port = ntohs(udp->src_port);
    uint16_t dest_port = ntohs(udp->dest_port);
    uint16_t udp_len = ntohs(udp->length);

    if (udp_len < UDP_HLEN || udp_len > length) {
        display_print("[UDP RX DROP] Bad udp_len="); display_print_dec(udp_len); display_print("\n");
        return;
    }

    // Verify UDP checksum if not 0 (RFC 768 allows checksum=0 to skip)
    if (udp->checksum != 0) {
        uint16_t ck = udp_calc_checksum(src_ip, dest_ip, payload, udp_len);
        if (ck != 0 && ck != 0xFFFF) {
            display_print("[UDP RX DROP] Bad UDP Checksum 0x"); display_print_hex(ck); display_print("\n");
            return;
        }
    }

    const uint8_t* udp_payload = payload + UDP_HLEN;
    uint16_t udp_payload_len = udp_len - UDP_HLEN;

    // Dispatch to registered port handler
    for (int i = 0; i < MAX_UDP_HANDLERS; i++) {
        if (g_udp_handlers[i].in_use && g_udp_handlers[i].port == dest_port) {
            g_udp_handlers[i].handler(src_ip, src_port, udp_payload, udp_payload_len);
            return;
        }
    }
}
