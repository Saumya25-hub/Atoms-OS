#include "icmp.h"
#include "kernel/net/ipv4/ipv4.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/net/netif.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint64_t val);

static IcmpPingResult g_ping_result = {0};

void icmp_init(void) {
    memset(&g_ping_result, 0, sizeof(IcmpPingResult));
}

bool icmp_send_ping(uint32_t target_ip, uint16_t id, uint16_t seq, const char* payload_str, uint16_t payload_len) {
    uint8_t buf[128];
    memset(buf, 0, sizeof(buf));

    struct icmp_hdr* icmp = (struct icmp_hdr*)buf;
    icmp->type = ICMP_TYPE_ECHO_REQUEST;
    icmp->code = ICMP_CODE_ECHO;
    icmp->checksum = 0;
    icmp->id = htons(id);
    icmp->seq = htons(seq);

    if (payload_str && payload_len > 0) {
        if (payload_len > (sizeof(buf) - 8)) payload_len = sizeof(buf) - 8;
        memcpy(buf + 8, payload_str, payload_len);
    }

    uint16_t icmp_len = 8 + payload_len;
    icmp->checksum = net_checksum(buf, icmp_len);

    display_print("[ICMP HEX DUMP] ");
    for (int b = 0; b < icmp_len; b++) {
        display_print_hex(buf[b]); display_print(" ");
    }
    display_print("\n");

    g_ping_result.req_checksum = icmp->checksum;
    g_ping_result.req_total_len = 20 + icmp_len;

    return ipv4_send(target_ip, IP_PROTO_ICMP, buf, icmp_len);
}

void icmp_process_packet(uint32_t src_ip, const uint8_t* payload, uint16_t length) {
    if (!payload || length < 8) {
        display_print("[ICMP RX DROP] payload null or len < 8\n");
        return;
    }

    const struct icmp_hdr* icmp = (const struct icmp_hdr*)payload;
    uint16_t ck = net_checksum(payload, length);
    if (ck != 0) {
        display_print("[ICMP RX DROP] Bad ICMP Checksum 0x"); display_print_hex(ck); display_print("\n");
        return;
    }

    display_print("[ICMP RX] Type="); display_print_dec(icmp->type); display_print(" Code="); display_print_dec(icmp->code);
    display_print(" ID="); display_print_hex(ntohs(icmp->id)); display_print(" Seq="); display_print_dec(ntohs(icmp->seq)); display_print("\n");

    if (icmp->type == ICMP_TYPE_ECHO_REPLY && icmp->code == ICMP_CODE_ECHO) {
        uint16_t rx_id = ntohs(icmp->id);
        uint16_t rx_seq = ntohs(icmp->seq);

        if (rx_id == g_ping_result.id && rx_seq == g_ping_result.seq) {
            g_ping_result.src_ip = src_ip;
            g_ping_result.icmp_checksum_pass = true;
            g_ping_result.ip_checksum_pass = true;
            g_ping_result.rx_icmp_checksum = icmp->checksum;
            g_ping_result.reply_received = true;
            display_print("[ICMP PING MATCH SUCCESS!]\n");
        } else {
            display_print("[ICMP RX DROP] ID/Seq mismatch\n");
        }
    } else if (icmp->type == ICMP_TYPE_ECHO_REQUEST && icmp->code == ICMP_CODE_ECHO) {
        NetInterface* netif = netif_get_default();
        if (!netif) return;

        // Generate unicast ICMP Echo Reply
        uint8_t reply_buf[256];
        if (length > sizeof(reply_buf)) return;

        memcpy(reply_buf, payload, length);
        struct icmp_hdr* reply_hdr = (struct icmp_hdr*)reply_buf;
        reply_hdr->type = ICMP_TYPE_ECHO_REPLY;
        reply_hdr->code = ICMP_CODE_ECHO;
        reply_hdr->checksum = 0;
        reply_hdr->checksum = net_checksum(reply_buf, length);

        ipv4_send(src_ip, IP_PROTO_ICMP, reply_buf, length);
    }
}

bool icmp_ping_target(uint32_t target_ip, uint16_t id, uint16_t seq, IcmpPingResult* out_result) {
    memset(&g_ping_result, 0, sizeof(IcmpPingResult));
    g_ping_result.target_ip = target_ip;
    g_ping_result.id = id;
    g_ping_result.seq = seq;

    const char default_payload[] = "ATOMS_OS_PHASE5_ICMP_PING_REPLY!";

    for (int retry = 0; retry < 5; retry++) {
        if (!icmp_send_ping(target_ip, id, seq, default_payload, 32)) {
            continue;
        }

        // Service E1000 RX ring with bounded polling loop (~5,000,000 iterations with I/O yield)
        E1000Frame frame;
        for (volatile int poll = 0; poll < 5000000; poll++) {
            io_in8(0x80); // Force QEMU TCG I/O exit to yield to host SLIRP event loop
            if (e1000_poll_receive(&frame)) {
                g_ping_result.rx_frame_len = frame.length;
                g_ping_result.rx_ethertype = ((uint16_t)frame.data[12] << 8) | frame.data[13];
                memcpy(g_ping_result.rx_src_mac, &frame.data[6], 6);
                g_ping_result.rx_desc_idx = 0;

                ethernet_process_frame(frame.data, frame.length);

                if (g_ping_result.reply_received) {
                    if (out_result) {
                        *out_result = g_ping_result;
                    }
                    return true;
                }
            }
        }
    }

    if (out_result) {
        *out_result = g_ping_result;
    }
    return g_ping_result.reply_received;
}
