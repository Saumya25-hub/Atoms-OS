#include "tls.h"
#include "kernel/net/tcp/tcp.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/core/lib/include/string.h"
#include "arch/x86_64/io/port_io.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint64_t val);

static TlsConnection g_tls_connections[4] = {0};

void tls_init(void) {
    memset(g_tls_connections, 0, sizeof(g_tls_connections));
}

bool tls_connect(uint32_t remote_ip, uint16_t remote_port, const char* sni_hostname, TlsConnection** tls_out) {
    if (remote_ip == 0 || remote_port == 0) return false;

    TlsConnection* tls = NULL;
    for (int i = 0; i < 4; i++) {
        if (!g_tls_connections[i].in_use) {
            tls = &g_tls_connections[i];
            break;
        }
    }

    if (!tls) return false;

    memset(tls, 0, sizeof(TlsConnection));
    tls->in_use = true;
    tls->state = TLS_STATE_IDLE;
    if (sni_hostname) {
        strncpy(tls->sni_hostname, sni_hostname, sizeof(tls->sni_hostname) - 1);
    }

    // 1. Establish underlying TCP 3-Way Handshake
    if (!tcp_connect(remote_ip, remote_port, &tls->tcp_conn) || !tls->tcp_conn) {
        tls->in_use = false;
        return false;
    }

    // 2. Build TLS ClientHello record
    uint8_t ch_buf[1024];
    size_t ch_len = 0;
    if (!tls_build_client_hello(tls, ch_buf, sizeof(ch_buf), &ch_len)) {
        tcp_close(tls->tcp_conn);
        tls->in_use = false;
        return false;
    }

    // 3. Transmit TLS ClientHello via TCP stream
    if (tcp_send(tls->tcp_conn, ch_buf, ch_len) <= 0) {
        tcp_close(tls->tcp_conn);
        tls->in_use = false;
        return false;
    }

    tls->state = TLS_STATE_CLIENT_HELLO_SENT;

    // 4. Poll E1000 RX Ring for ServerHello response over hardware DMA
    E1000Frame frame;
    uint8_t rx_tmp[1500];

    for (volatile int poll = 0; poll < 60000000; poll++) {
        io_in8(0x80);

        if (e1000_poll_receive(&frame)) {
            ethernet_process_frame(frame.data, frame.length);
        }

        tcp_check_retransmit(tls->tcp_conn);

        size_t avail = tcp_available(tls->tcp_conn);
        if (avail > 0) {
            int read_bytes = tcp_recv(tls->tcp_conn, rx_tmp, sizeof(rx_tmp));
            if (read_bytes > 0) {
                display_print("[TLS DATA RX] Bytes = "); display_print_dec(read_bytes); display_print("\n");
                if (tls->rx_len + read_bytes < sizeof(tls->rx_buf)) {
                    memcpy(tls->rx_buf + tls->rx_len, rx_tmp, read_bytes);
                    tls->rx_len += read_bytes;
                }
            }
        }

        // Parse TLS Record Header
        if (tls->rx_len >= sizeof(struct tls_record_hdr)) {
            struct tls_record_hdr hdr;
            if (tls_parse_record_header(tls->rx_buf, tls->rx_len, &hdr)) {
                tls->record_rcvd = true;
                display_print("[TLS RECORD HDR] Type="); display_print_dec(hdr.type);
                display_print(" Ver=0x"); display_print_hex(hdr.version);
                display_print(" Len="); display_print_dec(hdr.length); display_print("\n");

                if (hdr.type == TLS_CONTENT_ALERT && tls->rx_len >= sizeof(hdr) + 2) {
                    const uint8_t* alert_payload = tls->rx_buf + sizeof(hdr);
                    display_print("[TLS ALERT RX] Level="); display_print_dec(alert_payload[0]);
                    display_print(" Description="); display_print_dec(alert_payload[1]); display_print("\n");
                    break;
                }

                if (hdr.type == TLS_CONTENT_HANDSHAKE && tls->rx_len >= sizeof(hdr) + hdr.length) {
                    const uint8_t* hs_payload = tls->rx_buf + sizeof(hdr);
                    bool parsed = tls_parse_server_hello(tls, hs_payload, hdr.length);
                    display_print("[TLS HANDSHAKE PARSE] Result="); display_print(parsed ? "PASS\n" : "FAIL\n");
                    if (tls->server_hello_rcvd) {
                        break;
                    }
                }
            }
        }

        if (tls->tcp_conn->fin_received || tls->tcp_conn->rst_received) {
            break;
        }
    }

    if (tls_out) *tls_out = tls;
    return (tls->record_rcvd || tls->server_hello_rcvd);
}

int tls_send(TlsConnection* tls, const void* data, size_t len) {
    if (!tls || !tls->tcp_conn) return -1;
    // Transmit application record
    return tcp_send(tls->tcp_conn, data, len);
}

int tls_recv(TlsConnection* tls, void* buf, size_t max_len) {
    if (!tls || !tls->tcp_conn) return -1;
    return tcp_recv(tls->tcp_conn, buf, max_len);
}

void tls_close(TlsConnection* tls) {
    if (!tls || !tls->in_use) return;
    if (tls->tcp_conn) {
        tcp_close(tls->tcp_conn);
    }
    tls->in_use = false;
    tls->state = TLS_STATE_CLOSED;
}
