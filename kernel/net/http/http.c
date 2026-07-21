#include "http.h"
#include "kernel/net/dns/dns.h"
#include "kernel/net/tcp/tcp.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/core/lib/include/string.h"
#include "arch/x86_64/io/port_io.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint64_t val);

static int parse_dec_str(const char* str) {
    int val = 0;
    while (*str >= '0' && *str <= '9') {
        val = val * 10 + (*str - '0');
        str++;
    }
    return val;
}

static const char* str_find(const char* haystack, size_t haystack_len, const char* needle) {
    size_t needle_len = strlen(needle);
    if (needle_len > haystack_len) return NULL;

    for (size_t i = 0; i <= haystack_len - needle_len; i++) {
        if (memcmp(haystack + i, needle, needle_len) == 0) {
            return haystack + i;
        }
    }
    return NULL;
}

bool http_get(const char* hostname, const char* path, HttpResponse* resp) {
    if (!hostname || !resp) return false;
    if (!path) path = "/";

    memset(resp, 0, sizeof(HttpResponse));

    // 1. Resolve Hostname via Phase 7 DNS Engine
    uint32_t server_ip = 0;
    if (!dns_resolve_ipv4(hostname, &server_ip) || server_ip == 0) {
        return false;
    }

    // 2. Open TCP Connection to Remote IP:80
    TcpConnection* conn = NULL;
    if (!tcp_connect(server_ip, 80, &conn) || !conn) {
        return false;
    }

    // 3. Format HTTP/1.1 GET Request
    char req[512];
    memset(req, 0, sizeof(req));

    strcpy(req, "GET ");
    strcat(req, path);
    strcat(req, " HTTP/1.1\r\nHost: ");
    strcat(req, hostname);
    strcat(req, "\r\nUser-Agent: ATOMS-OS/1.0\r\nAccept: */*\r\nConnection: close\r\n\r\n");

    size_t req_len = strlen(req);

    // 4. Transmit HTTP Request via TCP Stream
    if (tcp_send(conn, req, req_len) <= 0) {
        return false;
    }

    // 5. Poll E1000 RX Ring and consume TCP Stream Response
    E1000Frame frame;
    uint8_t rx_tmp[1024];

    for (volatile int poll = 0; poll < 60000000; poll++) {
        io_in8(0x80);

        if (e1000_poll_receive(&frame)) {
            ethernet_process_frame(frame.data, frame.length);
        }

        size_t avail = tcp_available(conn);
        if (avail > 0) {
            int read_bytes = tcp_recv(conn, rx_tmp, sizeof(rx_tmp));
            if (read_bytes > 0) {
                if (resp->raw_len + read_bytes < sizeof(resp->raw_buf)) {
                    memcpy(resp->raw_buf + resp->raw_len, rx_tmp, read_bytes);
                    resp->raw_len += read_bytes;
                }
            }
        }

        // Check if Header boundary \r\n\r\n is reached
        if (!resp->header_complete && resp->raw_len >= 4) {
            const char* hdr_end = str_find((const char*)resp->raw_buf, resp->raw_len, "\r\n\r\n");
            if (hdr_end) {
                resp->header_complete = true;
                resp->headers_len = (size_t)(hdr_end - (const char*)resp->raw_buf) + 4;

                // Parse Status Line: e.g. "HTTP/1.1 200 OK" or "HTTP/1.1 301 Moved"
                const char* sp1 = str_find((const char*)resp->raw_buf, resp->raw_len, " ");
                if (sp1 && (size_t)(sp1 - (const char*)resp->raw_buf) < 20) {
                    resp->status_code = parse_dec_str(sp1 + 1);
                }

                // Check headers
                if (str_find((const char*)resp->raw_buf, resp->headers_len, "Transfer-Encoding: chunked") ||
                    str_find((const char*)resp->raw_buf, resp->headers_len, "transfer-encoding: chunked")) {
                    resp->is_chunked = true;
                }

                const char* cl_hdr = str_find((const char*)resp->raw_buf, resp->headers_len, "Content-Length: ");
                if (!cl_hdr) cl_hdr = str_find((const char*)resp->raw_buf, resp->headers_len, "content-length: ");
                if (cl_hdr) {
                    resp->content_length = (size_t)parse_dec_str(cl_hdr + 16);
                }
            }
        }

        if (resp->header_complete) {
            size_t body_avail = resp->raw_len - resp->headers_len;
            if (body_avail > 0) {
                size_t copy_len = (body_avail < sizeof(resp->body_buf)) ? body_avail : sizeof(resp->body_buf);
                memcpy(resp->body_buf, resp->raw_buf + resp->headers_len, copy_len);
                resp->body_len = copy_len;
            }

            // Break if response is complete (e.g. FIN received, or content length reached, or body received)
            if (conn->fin_received || conn->rst_received || resp->body_len > 0) {
                break;
            }
        }
    }

    return (resp->header_complete && resp->status_code > 0);
}
