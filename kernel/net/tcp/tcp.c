#include "tcp.h"
#include "kernel/net/ipv4/ipv4.h"
#include "kernel/net/netif.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/core/lib/include/string.h"
#include "arch/x86_64/io/port_io.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint64_t val);

static TcpConnection g_tcp_connections[MAX_TCP_CONNECTIONS] = {0};
static uint16_t g_ephemeral_counter = 0;
static uint32_t g_isn_counter = 0x10000000;

static const TcpConnection* g_last_conn_ptr = NULL;

void tcp_init(void) {
    memset(g_tcp_connections, 0, sizeof(g_tcp_connections));
    g_ephemeral_counter = 0;
    g_last_conn_ptr = NULL;
}

const TcpConnection* tcp_get_last_connection(void) {
    return g_last_conn_ptr;
}

static uint32_t tcp_generate_isn(void) {
    g_isn_counter += 0x01020304;
    if (g_isn_counter == 0) g_isn_counter = 0x10000000;
    return g_isn_counter;
}

static uint16_t tcp_alloc_ephemeral_port(void) {
    for (int retry = 0; retry < 16384; retry++) {
        uint16_t port = 49152 + (g_ephemeral_counter++ % 16384);
        bool in_use = false;
        for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
            if (g_tcp_connections[i].in_use && g_tcp_connections[i].local_port == port) {
                in_use = true;
                break;
            }
        }
        if (!in_use) return port;
    }
    return 49152;
}

uint16_t tcp_calc_checksum(uint32_t src_ip, uint32_t dest_ip, const void* tcp_data, uint16_t tcp_len) {
    struct tcp_pseudo_hdr pseudo;
    pseudo.src_ip = src_ip;
    pseudo.dest_ip = dest_ip;
    pseudo.zero = 0;
    pseudo.protocol = IP_PROTO_TCP;
    pseudo.tcp_len = htons(tcp_len);

    uint32_t sum = 0;
    const uint8_t* p_bytes = (const uint8_t*)&pseudo;
    for (uint16_t i = 0; i < sizeof(struct tcp_pseudo_hdr); i += 2) {
        sum += ((uint16_t)p_bytes[i] << 8) | p_bytes[i + 1];
    }

    const uint8_t* t_bytes = (const uint8_t*)tcp_data;
    for (uint16_t i = 0; i < (tcp_len & ~1U); i += 2) {
        sum += ((uint16_t)t_bytes[i] << 8) | t_bytes[i + 1];
    }

    if (tcp_len & 1) {
        sum += ((uint16_t)t_bytes[tcp_len - 1] << 8);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    uint16_t ck = htons((uint16_t)(~sum));
    return (ck == 0) ? 0xFFFF : ck;
}

static bool tcp_send_segment(TcpConnection* conn, uint8_t flags, const void* payload, uint16_t payload_len) {
    if (!conn) return false;

    uint16_t header_len = sizeof(struct tcp_hdr); // 20 bytes
    uint16_t total_len = header_len + payload_len;

    if (total_len > 1460) return false;

    uint8_t buf[1500];
    memset(buf, 0, sizeof(buf));

    struct tcp_hdr* tcp = (struct tcp_hdr*)buf;
    tcp->src_port = htons(conn->local_port);
    tcp->dest_port = htons(conn->remote_port);
    tcp->seq_num = htonl(conn->snd_nxt);
    tcp->ack_num = (flags & TCP_FLAG_ACK) ? htonl(conn->rcv_nxt) : 0;
    tcp->data_offset_reserved = (5 << 4); // Data Offset = 5 (20 bytes)
    tcp->flags = flags;
    tcp->window = htons(conn->rcv_wnd);
    tcp->checksum = 0;
    tcp->urgent_ptr = 0;

    if (payload && payload_len > 0) {
        memcpy(buf + header_len, payload, payload_len);
    }

    tcp->checksum = tcp_calc_checksum(conn->local_ip, conn->remote_ip, buf, total_len);

    return ipv4_send(conn->remote_ip, IP_PROTO_TCP, buf, total_len);
}

void tcp_process_packet(uint32_t src_ip, uint32_t dest_ip, const uint8_t* payload, uint16_t length) {
    if (!payload || length < TCP_MIN_HLEN) {
        return;
    }

    const struct tcp_hdr* tcp = (const struct tcp_hdr*)payload;
    uint8_t doff = (tcp->data_offset_reserved >> 4);
    uint16_t hlen = doff * 4;

    if (doff < 5 || hlen > length) {
        return;
    }

    // Verify TCP Pseudo-Header Checksum
    uint16_t ck = tcp_calc_checksum(src_ip, dest_ip, payload, length);
    if (ck != 0 && ck != 0xFFFF) {
        display_print("[TCP RX DROP] Bad Checksum 0x"); display_print_hex(ck); display_print("\n");
        return;
    }

    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);
    uint32_t seq_num = ntohl(tcp->seq_num);
    uint32_t ack_num = ntohl(tcp->ack_num);

    // 4-Tuple Matching
    TcpConnection* conn = NULL;
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (g_tcp_connections[i].in_use &&
            g_tcp_connections[i].local_ip == dest_ip &&
            g_tcp_connections[i].remote_ip == src_ip &&
            g_tcp_connections[i].local_port == dest_port &&
            g_tcp_connections[i].remote_port == src_port) {
            conn = &g_tcp_connections[i];
            break;
        }
    }

    if (!conn) {
        return;
    }

    conn->rx_seq = seq_num;
    conn->rx_ack = ack_num;
    conn->rx_flags = tcp->flags;

    // Active Open Handshake Processing
    if (conn->state == TCP_STATE_SYN_SENT) {
        if (tcp->flags & TCP_FLAG_RST) {
            conn->rst_received = true;
            conn->state = TCP_STATE_CLOSED;
            return;
        }

        // Validate SYN + ACK flags and expected ACK number
        if ((tcp->flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) == (TCP_FLAG_SYN | TCP_FLAG_ACK)) {
            if (ack_num == conn->snd_nxt) {
                conn->rcv_nxt = seq_num + 1;
                conn->snd_una = ack_num;
                conn->snd_wnd = ntohs(tcp->window);
                conn->syn_ack_received = true;

                // Send Final ACK to complete 3-way handshake
                tcp_send_segment(conn, TCP_FLAG_ACK, NULL, 0);

                conn->state = TCP_STATE_ESTABLISHED;
            }
        }
    }
}

bool tcp_connect(uint32_t remote_ip, uint16_t remote_port, TcpConnection** conn_out) {
    if (remote_ip == 0 || remote_port == 0) return false;

    NetInterface* netif = netif_get_default();
    if (!netif || netif->state != NETIF_STATE_CONFIGURED) {
        return false;
    }

    // Allocate free TcpConnection slot
    TcpConnection* conn = NULL;
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (!g_tcp_connections[i].in_use) {
            conn = &g_tcp_connections[i];
            break;
        }
    }

    if (!conn) return false;

    memset(conn, 0, sizeof(TcpConnection));
    conn->local_ip = netif->ip_addr;
    conn->remote_ip = remote_ip;
    conn->local_port = tcp_alloc_ephemeral_port();
    conn->remote_port = remote_port;
    conn->state = TCP_STATE_CLOSED;
    conn->rcv_wnd = 8192;
    conn->in_use = true;

    g_last_conn_ptr = conn;
    if (conn_out) *conn_out = conn;

    // Generate ISN and set sequence tracking
    conn->isn = tcp_generate_isn();
    conn->snd_nxt = conn->isn;
    conn->snd_una = conn->isn;

    // 1. Transmit SYN segment
    bool syn_sent = tcp_send_segment(conn, TCP_FLAG_SYN, NULL, 0);
    if (!syn_sent) {
        conn->in_use = false;
        return false;
    }

    conn->snd_nxt = conn->isn + 1;
    conn->state = TCP_STATE_SYN_SENT;

    // 2. Poll E1000 RX DMA Ring for SYN-ACK completion
    E1000Frame frame;
    for (volatile int poll = 0; poll < 50000000; poll++) {
        io_in8(0x80);
        if (e1000_poll_receive(&frame)) {
            ethernet_process_frame(frame.data, frame.length);
            if (conn->state == TCP_STATE_ESTABLISHED || conn->rst_received) break;
        }
    }

    return (conn->state == TCP_STATE_ESTABLISHED);
}
