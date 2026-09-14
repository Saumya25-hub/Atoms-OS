#include "http.h"
#include "kernel/net/dns/dns.h"
#include "kernel/net/tcp/tcp.h"
#include "kernel/net/tls/tls.h"
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

bool http_decode_chunked(const uint8_t* raw_body, size_t raw_body_len, uint8_t* out_body, size_t max_out, size_t* decoded_len) {
    if (!raw_body || !out_body || !decoded_len) return false;

    size_t in_pos = 0;
    size_t out_pos = 0;

    while (in_pos < raw_body_len) {
        size_t line_end = 0;
        bool found_line = false;
        for (size_t i = in_pos; i + 1 < raw_body_len; i++) {
            if (raw_body[i] == '\r' && raw_body[i + 1] == '\n') {
                line_end = i;
                found_line = true;
                break;
            }
        }
        if (!found_line) break;

        size_t chunk_size = 0;
        for (size_t i = in_pos; i < line_end; i++) {
            char c = (char)raw_body[i];
            if (c == ';') break;
            int digit = -1;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
            else break;
            chunk_size = (chunk_size << 4) | (size_t)digit;
        }

        in_pos = line_end + 2;

        if (chunk_size == 0) {
            *decoded_len = out_pos;
            return true;
        }

        size_t copy_bytes = chunk_size;
        if (in_pos + copy_bytes > raw_body_len) {
            copy_bytes = raw_body_len - in_pos;
        }
        if (out_pos + copy_bytes > max_out) {
            copy_bytes = max_out - out_pos;
        }

        if (copy_bytes > 0) {
            memcpy(out_body + out_pos, raw_body + in_pos, copy_bytes);
            out_pos += copy_bytes;
        }

        in_pos += chunk_size;

        if (in_pos + 1 < raw_body_len && raw_body[in_pos] == '\r' && raw_body[in_pos + 1] == '\n') {
            in_pos += 2;
        }
    }

    *decoded_len = out_pos;
    return (out_pos > 0);
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

    // 5. Poll RX Ring and consume TCP Stream Response
    extern bool net_poll(void);
    uint8_t rx_tmp[1024];

    for (volatile int poll = 0; poll < 60000000; poll++) {
        io_in8(0x80);

        net_poll();

        tcp_check_retransmit(conn);

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

        if (!resp->header_complete && resp->raw_len >= 4) {
            const char* hdr_end = str_find((const char*)resp->raw_buf, resp->raw_len, "\r\n\r\n");
            if (hdr_end) {
                resp->header_complete = true;
                resp->headers_len = (size_t)(hdr_end - (const char*)resp->raw_buf) + 4;

                const char* sp1 = str_find((const char*)resp->raw_buf, resp->raw_len, " ");
                if (sp1 && (size_t)(sp1 - (const char*)resp->raw_buf) < 20) {
                    resp->status_code = parse_dec_str(sp1 + 1);
                }

                if (str_find((const char*)resp->raw_buf, resp->headers_len, "Transfer-Encoding: chunked") ||
                    str_find((const char*)resp->raw_buf, resp->headers_len, "transfer-encoding: chunked")) {
                    resp->is_chunked = true;
                    resp->body_mode = HTTP_BODY_MODE_CHUNKED;
                }

                const char* cl_hdr = str_find((const char*)resp->raw_buf, resp->headers_len, "Content-Length: ");
                if (!cl_hdr) cl_hdr = str_find((const char*)resp->raw_buf, resp->headers_len, "content-length: ");
                if (cl_hdr) {
                    resp->content_length = (size_t)parse_dec_str(cl_hdr + 16);
                    if (!resp->is_chunked) resp->body_mode = HTTP_BODY_MODE_CONTENT_LENGTH;
                }
            }
        }

        if (resp->header_complete) {
            size_t raw_body_len = resp->raw_len - resp->headers_len;
            if (resp->is_chunked) {
                http_decode_chunked(resp->raw_buf + resp->headers_len, raw_body_len, resp->body_buf, sizeof(resp->body_buf), &resp->body_len);
            } else {
                size_t copy_len = (raw_body_len < sizeof(resp->body_buf)) ? raw_body_len : sizeof(resp->body_buf);
                memcpy(resp->body_buf, resp->raw_buf + resp->headers_len, copy_len);
                resp->body_len = copy_len;
            }

            if (conn->fin_received || conn->rst_received || resp->body_len > 0) {
                break;
            }
        }
    }

    tcp_close(conn);
    return (resp->header_complete && resp->status_code > 0);
}

bool https_get(const char* hostname, const char* path, HttpResponse* resp) {
    if (!hostname || !resp) return false;
    if (!path) path = "/";

    memset(resp, 0, sizeof(HttpResponse));

    uint32_t server_ip = 0;
    if (!dns_resolve_ipv4(hostname, &server_ip) || server_ip == 0) {
        return false;
    }

    TlsConnection* tls = NULL;
    if (!tls_connect(server_ip, 443, hostname, &tls) || !tls) {
        return false;
    }

    char req[512];
    memset(req, 0, sizeof(req));
    strcpy(req, "GET ");
    strcat(req, path);
    strcat(req, " HTTP/1.1\r\nHost: ");
    strcat(req, hostname);
    strcat(req, "\r\nUser-Agent: ATOMS-OS/1.0\r\nAccept: */*\r\nConnection: close\r\n\r\n");

    size_t req_len = strlen(req);
    if (req_len > 0) {
        tls_send(tls, req, req_len);
    }

    extern uint32_t timer_get_ticks(void);
    uint32_t start_tick = timer_get_ticks();
    uint8_t rx_buf[2048];
    extern bool net_poll(void);

    while ((timer_get_ticks() - start_tick) < 2500) {
        net_poll();

        int dec_bytes = tls_recv(tls, rx_buf, sizeof(rx_buf));
        if (dec_bytes > 0) {
            if (resp->raw_len + dec_bytes < sizeof(resp->raw_buf)) {
                memcpy(resp->raw_buf + resp->raw_len, rx_buf, dec_bytes);
                resp->raw_len += dec_bytes;
                resp->raw_buf[resp->raw_len] = '\0';
            }
        }

        if (!resp->header_complete && resp->raw_len >= 4) {
            const char* hdr_end = str_find((const char*)resp->raw_buf, resp->raw_len, "\r\n\r\n");
            if (hdr_end) {
                resp->header_complete = true;
                resp->headers_len = (size_t)(hdr_end - (const char*)resp->raw_buf) + 4;

                const char* sp1 = str_find((const char*)resp->raw_buf, resp->raw_len, " ");
                if (sp1 && (size_t)(sp1 - (const char*)resp->raw_buf) < 20) {
                    resp->status_code = parse_dec_str(sp1 + 1);
                }

                if (str_find((const char*)resp->raw_buf, resp->headers_len, "Transfer-Encoding: chunked") ||
                    str_find((const char*)resp->raw_buf, resp->headers_len, "transfer-encoding: chunked")) {
                    resp->is_chunked = true;
                    resp->body_mode = HTTP_BODY_MODE_CHUNKED;
                }

                const char* cl_hdr = str_find((const char*)resp->raw_buf, resp->headers_len, "Content-Length: ");
                if (!cl_hdr) cl_hdr = str_find((const char*)resp->raw_buf, resp->headers_len, "content-length: ");
                if (cl_hdr) {
                    resp->content_length = (size_t)parse_dec_str(cl_hdr + 16);
                    if (!resp->is_chunked) resp->body_mode = HTTP_BODY_MODE_CONTENT_LENGTH;
                }
            }
        }

        if (resp->header_complete) {
            size_t raw_body_len = resp->raw_len - resp->headers_len;
            if (resp->is_chunked) {
                http_decode_chunked(resp->raw_buf + resp->headers_len, raw_body_len, resp->body_buf, sizeof(resp->body_buf), &resp->body_len);
            } else {
                size_t copy_len = (raw_body_len < sizeof(resp->body_buf)) ? raw_body_len : sizeof(resp->body_buf);
                memcpy(resp->body_buf, resp->raw_buf + resp->headers_len, copy_len);
                resp->body_len = copy_len;
            }
            if (resp->body_len > 0) break;
        }

        if (tls->tcp_conn->fin_received || tls->tcp_conn->rst_received) {
            break;
        }
    }

    bool ok = (tls->record_rcvd || tls->server_hello_rcvd || resp->status_code > 0);
    tls_close(tls);
    return ok;
}
