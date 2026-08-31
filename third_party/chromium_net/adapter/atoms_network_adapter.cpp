/*
 * ATOMS OS — Chromium Network Adapter
 * Bridges Chromium URLLoader to ATOMS kernel networking (DNS, TCP, TLS, HTTP)
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * This adapter calls the REAL ATOMS kernel networking ABI:
 *   ABE_NetDNS_Resolve → real UDP port 53
 *   ABE_NetConn_Open   → real TCP 3-way handshake
 *   ABE_NetConn_Send/Recv → real socket I/O
 *   ABE_NetHTTP_SerializeRequest → HTTP/1.1 wire format
 *   ABE TLS 1.2 ECDHE  → real TLS handshake (for HTTPS)
 */

#include "atoms_network_adapter.h"

// ATOMS kernel networking C ABI
extern "C" {
#include "kernel/browser_engine/network/abe_net_dns.h"
#include "kernel/browser_engine/network/abe_net_conn.h"
#include "kernel/browser_engine/network/abe_net_http.h"

__attribute__((weak)) ABE_Error ABE_NetDNS_Resolve(const char* hostname, uint32_t* out_ip) {
    (void)hostname; (void)out_ip;
    return ABE_ERR_NET_DNS_FAILED;
}

__attribute__((weak)) ABE_Error ABE_NetConn_Open(const char* host, uint16_t port, bool use_tls, ABE_ConnHandle* out_conn) {
    (void)host; (void)port; (void)use_tls; (void)out_conn;
    return ABE_ERR_NET_CONNECT_FAILED;
}

__attribute__((weak)) ABE_Error ABE_NetConn_Close(ABE_ConnHandle handle) {
    (void)handle;
    return ABE_SUCCESS;
}

__attribute__((weak)) ABE_Error ABE_NetConn_Send(ABE_ConnHandle handle, const void* data, size_t len, size_t* out_sent) {
    (void)handle; (void)data; (void)len; (void)out_sent;
    return ABE_ERR_NET_SEND_FAILED;
}

__attribute__((weak)) ABE_Error ABE_NetConn_Recv(ABE_ConnHandle handle, void* buf, size_t max_len, size_t* out_rcvd) {
    (void)handle; (void)buf; (void)max_len; (void)out_rcvd;
    return ABE_ERR_NET_RECV_FAILED;
}

__attribute__((weak)) ABE_Error ABE_NetHTTP_CreateRequest(ABE_HTTPMethod method, const char* url_str, ABE_HTTPRequest* out_req) {
    (void)method; (void)url_str; (void)out_req;
    return ABE_SUCCESS;
}

__attribute__((weak)) ABE_Error ABE_NetHTTP_AddHeader(ABE_HTTPRequest* req, const char* name, const char* value) {
    (void)req; (void)name; (void)value;
    return ABE_SUCCESS;
}

__attribute__((weak)) ABE_Error ABE_NetHTTP_SerializeRequest(const ABE_HTTPRequest* req, uint8_t** out_buf, size_t* out_len) {
    (void)req; (void)out_buf; (void)out_len;
    return ABE_ERR_NET_PARSE_FAILED;
}

__attribute__((weak)) void ABE_NetHTTP_FreeRequest(ABE_HTTPRequest* req) {
    (void)req;
}
}

namespace net {

/*
 * Parse raw HTTP response bytes into URLLoaderResult.
 * Expects standard HTTP/1.1 response: status line + headers + \r\n\r\n + body.
 */
static bool ParseRawHTTPResponse(const std::string& raw, URLLoaderResult* result) {
    // Find end of headers
    size_t header_end = raw.find("\r\n\r\n");
    if (header_end == (size_t)-1) {
        // Try single \n\n as fallback
        header_end = raw.find("\n\n");
        if (header_end == (size_t)-1) return false;
        size_t body_start = header_end + 2;
        std::string headers_section = raw.substr(0, header_end);
        result->response_body = raw.substr(body_start);
        result->response_headers = HttpResponseHeaders(headers_section);
        result->http_status_code = result->response_headers.response_code();
        return true;
    }
    size_t body_start = header_end + 4;
    std::string headers_section = raw.substr(0, header_end);
    result->response_body = raw.substr(body_start);
    result->response_headers = HttpResponseHeaders(headers_section);
    result->http_status_code = result->response_headers.response_code();
    return true;
}

URLLoaderResult AtomsNetworkAdapter_Fetch(const GURL& url, const HttpRequestHeaders& headers) {
    URLLoaderResult result;
    result.net_error = NET_OK;
    result.http_status_code = 0;
    result.response_body = "";
    result.final_url = url.spec();
    result.from_cache = false;

    if (!url.is_valid()) {
        result.net_error = ERR_INVALID_URL;
        return result;
    }

    // 1. DNS resolution via ATOMS kernel
    uint32_t resolved_ip = 0;
    ABE_Error dns_err = ABE_NetDNS_Resolve(url.host().c_str(), &resolved_ip);
    if (dns_err != ABE_SUCCESS) {
        result.net_error = ERR_NAME_NOT_RESOLVED;
        return result;
    }

    // 2. Determine port and TLS
    bool use_tls = url.is_secure();
    uint16_t port = url.port();

    // 3. Open TCP connection via ATOMS kernel (TLS handshake is automatic for HTTPS)
    ABE_ConnHandle conn_handle;
    ABE_Error conn_err = ABE_NetConn_Open(url.host().c_str(), port, use_tls, &conn_handle);
    if (conn_err != ABE_SUCCESS) {
        result.net_error = ERR_CONNECTION_REFUSED;
        return result;
    }

    // 4. Build HTTP/1.1 request
    ABE_HTTPRequest http_req;
    ABE_Error req_err = ABE_NetHTTP_CreateRequest(ABE_HTTP_METHOD_GET, url.spec().c_str(), &http_req);
    if (req_err != ABE_SUCCESS) {
        ABE_NetConn_Close(conn_handle);
        result.net_error = ERR_FAILED;
        return result;
    }

    // 5. Copy Chromium request headers into ABE request
    ABE_NetHTTP_AddHeader(&http_req, "Host", url.host().c_str());
    ABE_NetHTTP_AddHeader(&http_req, "Connection", "keep-alive");
    ABE_NetHTTP_AddHeader(&http_req, "User-Agent", "ATRIX/1.0 (ATOMS OS; Chromium Net)");

    const std::vector<HeaderKeyValuePair>& extra = headers.GetHeaders();
    for (size_t i = 0; i < extra.size(); i++) {
        ABE_NetHTTP_AddHeader(&http_req, extra[i].key.c_str(), extra[i].value.c_str());
    }

    // 6. Serialize to wire format
    uint8_t* wire_buf = nullptr;
    size_t wire_len = 0;
    ABE_Error ser_err = ABE_NetHTTP_SerializeRequest(&http_req, &wire_buf, &wire_len);
    if (ser_err != ABE_SUCCESS || !wire_buf) {
        ABE_NetHTTP_FreeRequest(&http_req);
        ABE_NetConn_Close(conn_handle);
        result.net_error = ERR_FAILED;
        return result;
    }

    // 7. Send over TCP / TLS channel
    size_t total_sent = 0;
    while (total_sent < wire_len) {
        size_t sent = 0;
        ABE_Error send_err = ABE_NetConn_Send(conn_handle, wire_buf + total_sent,
                                               wire_len - total_sent, &sent);
        if (send_err != ABE_SUCCESS) {
            ABE_NetHTTP_FreeRequest(&http_req);
            ABE_NetConn_Close(conn_handle);
            result.net_error = ERR_FAILED;
            return result;
        }
        total_sent += sent;
    }

    ABE_NetHTTP_FreeRequest(&http_req);

    // 8. Receive HTTP response bytes
    std::string response_raw = "";
    uint8_t recv_buf[4096];
    size_t rcvd = 0;

    // Read until we have complete headers + body
    while (true) {
        ABE_Error recv_err = ABE_NetConn_Recv(conn_handle, recv_buf, sizeof(recv_buf), &rcvd);
        if (recv_err != ABE_SUCCESS || rcvd == 0) break;
        response_raw.append((const char*)recv_buf, rcvd);

        // Check if we have complete headers
        size_t header_end_pos = response_raw.find("\r\n\r\n");
        if (header_end_pos != (size_t)-1) {
            // Check Content-Length to know when body is complete
            HttpResponseHeaders temp_headers(response_raw.substr(0, header_end_pos));
            int content_length = temp_headers.GetContentLength();
            if (content_length >= 0) {
                size_t body_start = header_end_pos + 4;
                size_t body_received = response_raw.size() - body_start;
                if (body_received >= (size_t)content_length) break;
            }
        }
    }

    // 9. Close connection
    ABE_NetConn_Close(conn_handle);

    // 10. Parse response
    if (response_raw.empty()) {
        result.net_error = ERR_EMPTY_RESPONSE;
        return result;
    }

    if (!ParseRawHTTPResponse(response_raw, &result)) {
        result.net_error = ERR_FAILED;
        return result;
    }

    return result;
}

} // namespace net
