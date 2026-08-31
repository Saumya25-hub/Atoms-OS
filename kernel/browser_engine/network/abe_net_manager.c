#include "abe_net_manager.h"
#include "../url/abe_url.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static ABE_NetworkManager g_net_mgr;
static uint32_t g_next_req_id = 4000;

ABE_Error ABE_NetManager_Init(void) {
    if (g_net_mgr.is_initialized) return ABE_ERR_ALREADY_INITIALIZED;
    memset(&g_net_mgr, 0, sizeof(ABE_NetworkManager));

    ABE_Error err;
    err = ABE_NetDNS_Init();
    if (err != ABE_SUCCESS) return err;

    err = ABE_NetTLS_Init();
    if (err != ABE_SUCCESS) return err;

    err = ABE_NetConn_Init();
    if (err != ABE_SUCCESS) return err;

    err = ABE_NetHTTP_Init();
    if (err != ABE_SUCCESS) return err;

    err = ABE_NetParser_Init();
    if (err != ABE_SUCCESS) return err;

    err = ABE_NetRedirect_Init();
    if (err != ABE_SUCCESS) return err;

    err = ABE_NetCompress_Init();
    if (err != ABE_SUCCESS) return err;

    err = ABE_NetDownload_Init();
    if (err != ABE_SUCCESS) return err;

    g_net_mgr.is_initialized = true;
    ABE_Log(ABE_LOG_INFO, "NET", "=========================================================");
    ABE_Log(ABE_LOG_INFO, "NET", " ABE Phase 2 Production Networking Engine Initialized   ");
    ABE_Log(ABE_LOG_INFO, "NET", "=========================================================");
    return ABE_SUCCESS;
}

ABE_Error ABE_NetManager_Shutdown(void) {
    if (!g_net_mgr.is_initialized) return ABE_ERR_NOT_INITIALIZED;

    for (uint32_t i = 0; i < ABE_MAX_NETWORK_REQUESTS; i++) {
        if (g_net_mgr.requests[i].state != REQ_STATE_IDLE) {
            ABE_FreeHTTPResponse(&g_net_mgr.requests[i].response);
            ABE_NetHTTP_FreeRequest(&g_net_mgr.requests[i].request);
            g_net_mgr.requests[i].state = REQ_STATE_IDLE;
        }
    }

    ABE_NetDownload_Shutdown();
    ABE_NetCompress_Shutdown();
    ABE_NetRedirect_Shutdown();
    ABE_NetParser_Shutdown();
    ABE_NetHTTP_Shutdown();
    ABE_NetConn_Shutdown();
    ABE_NetTLS_Shutdown();
    ABE_NetDNS_Shutdown();

    g_net_mgr.is_initialized = false;
    ABE_Log(ABE_LOG_INFO, "NET", "ABE Phase 2 Production Networking Engine shut down cleanly");
    return ABE_SUCCESS;
}

ABE_NetworkRequestNode* ABE_NetManager_GetNode(ABE_RequestHandle req) {
    if (!g_net_mgr.is_initialized || req == ABE_INVALID_HANDLE) return NULL;
    uint32_t slot = (req >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_NETWORK_REQUESTS) return NULL;
    if (g_net_mgr.requests[slot].handle == req && g_net_mgr.requests[slot].state != REQ_STATE_IDLE) {
        return &g_net_mgr.requests[slot];
    }
    return NULL;
}

ABE_Error ABE_NetManager_SendRequest(ABE_ConnHandle conn, const ABE_HTTPRequest* req, ABE_RequestHandle* out_req) {
    if (!g_net_mgr.is_initialized || conn == ABE_INVALID_HANDLE || !req || !out_req) return ABE_ERR_INVALID_PARAM;
    if (g_net_mgr.active_request_count >= ABE_MAX_NETWORK_REQUESTS) return ABE_ERR_RESOURCE_EXHAUSTED;

    uint32_t slot = ABE_INVALID_HANDLE;
    for (uint32_t i = 0; i < ABE_MAX_NETWORK_REQUESTS; i++) {
        if (g_net_mgr.requests[i].state == REQ_STATE_IDLE ||
            g_net_mgr.requests[i].state == REQ_STATE_COMPLETED ||
            g_net_mgr.requests[i].state == REQ_STATE_FAILED ||
            g_net_mgr.requests[i].state == REQ_STATE_CANCELLED) {
            slot = i;
            break;
        }
    }

    if (slot == ABE_INVALID_HANDLE) return ABE_ERR_RESOURCE_EXHAUSTED;

    // Serialize HTTP Request
    uint8_t* raw_req = NULL;
    size_t raw_req_len = 0;
    ABE_Error err = ABE_NetHTTP_SerializeRequest(req, &raw_req, &raw_req_len);
    if (err != ABE_SUCCESS) return err;

    // Transmit over socket/TLS connection
    size_t sent_bytes = 0;
    err = ABE_NetConn_Send(conn, raw_req, raw_req_len, &sent_bytes);
    kfree(raw_req);
    ABE_Diag_RecordMemoryFree(raw_req_len + 4096);

    if (err != ABE_SUCCESS) return err;

    ABE_NetworkRequestNode* node = &g_net_mgr.requests[slot];
    memset(node, 0, sizeof(ABE_NetworkRequestNode));
    node->handle = (g_next_req_id++) | (slot << 16);
    node->conn_handle = conn;
    node->request = *req;
    node->state = REQ_STATE_SENDING;
    node->start_time_us = 1000;

    g_net_mgr.active_request_count++;
    *out_req = node->handle;

    ABE_LogVal(ABE_LOG_INFO, "NET", "Sent HTTP Request via Network Engine, Request Handle: ", node->handle);
    return ABE_SUCCESS;
}

ABE_Error ABE_NetManager_ReadResponse(ABE_RequestHandle req, ABE_HTTPResponse* out_resp) {
    if (!g_net_mgr.is_initialized || req == ABE_INVALID_HANDLE || !out_resp) return ABE_ERR_INVALID_PARAM;
    ABE_NetworkRequestNode* node = ABE_NetManager_GetNode(req);
    if (!node) return ABE_ERR_INVALID_PARAM;

    node->state = REQ_STATE_RECEIVING;
    ABE_Error err = ABE_SUCCESS;

    // Allocate accumulation buffer (Max Phase 2 response ceiling: 64KB)
    size_t max_buf_len = 65536;
    uint8_t* rx_buf = (uint8_t*)kmalloc(max_buf_len + 1);
    if (!rx_buf) return ABE_ERR_OUT_OF_MEMORY;
    memset(rx_buf, 0, max_buf_len + 1);

    size_t total_rcvd = 0;
    extern uint32_t timer_get_ticks(void);
    uint32_t start_tick = timer_get_ticks();
    uint32_t timeout_ticks = 500; // 5.0 seconds timeout

    bool header_parsed = false;
    size_t header_bytes = 0;

    while ((timer_get_ticks() - start_tick) < timeout_ticks) {
        size_t chunk_rcvd = 0;
        size_t space_left = max_buf_len - total_rcvd;
        if (space_left == 0) {
            kfree(rx_buf);
            node->state = REQ_STATE_FAILED;
            return ABE_ERR_RESOURCE_EXHAUSTED;
        }

        uint8_t temp_chunk[1024];
        size_t read_limit = (space_left < sizeof(temp_chunk)) ? space_left : sizeof(temp_chunk);
        ABE_Error recv_err = ABE_NetConn_Recv(node->conn_handle, temp_chunk, read_limit, &chunk_rcvd);

        if (recv_err == ABE_SUCCESS && chunk_rcvd > 0) {
            memcpy(rx_buf + total_rcvd, temp_chunk, chunk_rcvd);
            total_rcvd += chunk_rcvd;
            rx_buf[total_rcvd] = '\0';
            start_tick = timer_get_ticks(); // Reset timeout on data arrival
        }

        // Check for complete headers
        if (!header_parsed && total_rcvd >= 4) {
            const char* hdr_end = strstr((const char*)rx_buf, "\r\n\r\n");
            if (hdr_end) {
                ABE_Error ph_err = ABE_NetParser_ParseHeader(rx_buf, total_rcvd, &node->header_info, &header_bytes);
                if (ph_err == ABE_SUCCESS) {
                    header_parsed = true;
                }
            }
        }

        // Check completion condition if headers are parsed
        if (header_parsed) {
            size_t body_rcvd = total_rcvd - header_bytes;

            if (node->header_info.content_length > 0) {
                if (body_rcvd >= node->header_info.content_length) {
                    break; // Complete Content-Length payload received
                }
            } else if (node->header_info.is_chunked) {
                const char* body_str = (const char*)(rx_buf + header_bytes);
                if (strstr(body_str, "0\r\n\r\n") != NULL || strstr(body_str, "\r\n0\r\n\r\n") != NULL) {
                    break; // Chunked EOF terminator reached
                }
            } else {
                // If connection closed or no content length and status is complete
                if (chunk_rcvd == 0 && total_rcvd > header_bytes) {
                    break;
                }
            }
        }
    }

    if (total_rcvd == 0 || !header_parsed) {
        kfree(rx_buf);
        node->state = REQ_STATE_FAILED;
        return (total_rcvd == 0) ? ABE_ERR_NET_RECV_FAILED : ABE_ERR_NET_PARSE_FAILED;
    }

    // Re-parse header accurately with full accumulated buffer
    err = ABE_NetParser_ParseHeader(rx_buf, total_rcvd, &node->header_info, &header_bytes);
    if (err != ABE_SUCCESS) {
        kfree(rx_buf);
        node->state = REQ_STATE_FAILED;
        return err;
    }

    // Process Redirects if 301, 302, 303, 307, 308
    if (ABE_NetRedirect_IsRedirectCode(node->header_info.status_code)) {
        char new_url[ABE_MAX_URL_LEN];
        ABE_HTTPMethod new_method = node->request.method;
        err = ABE_NetRedirect_ProcessRedirect(&node->redirect_history, node->request.url, node->header_info.status_code, node->header_info.location, node->request.method, new_url, &new_method);
        if (err == ABE_SUCCESS) {
            kfree(rx_buf);
            // Re-execute request to redirect target URL
            ABE_HTTPRequest redir_req;
            ABE_NetHTTP_CreateRequest(new_method, new_url, &redir_req);
            ABE_ConnHandle new_conn = ABE_INVALID_HANDLE;

            ABE_URL parsed_target;
            ABE_ParseURL(new_url, &parsed_target);
            ABE_OpenConnection(parsed_target.host, parsed_target.port, (parsed_target.scheme == ABE_SCHEME_HTTPS), &new_conn);

            ABE_RequestHandle new_req_handle = ABE_INVALID_HANDLE;
            ABE_NetManager_SendRequest(new_conn, &redir_req, &new_req_handle);
            return ABE_NetManager_ReadResponse(new_req_handle, out_resp);
        }
    }

    // Decompress Body if gzip/deflate encoded, or decode chunked
    uint8_t* final_body = NULL;
    size_t final_body_len = 0;

    if (node->header_info.is_chunked) {
        ABE_NetParser_DecodeChunked(node->header_info.raw_body_start, node->header_info.raw_body_len, &final_body, &final_body_len);
    } else if (node->header_info.is_gzipped) {
        ABE_NetCompress_Decompress(ABE_COMPRESS_GZIP, node->header_info.raw_body_start, node->header_info.raw_body_len, &final_body, &final_body_len);
    } else {
        final_body_len = node->header_info.raw_body_len;
        if (final_body_len > 0) {
            final_body = (uint8_t*)kmalloc(final_body_len + 1);
            if (final_body) {
                memcpy(final_body, node->header_info.raw_body_start, final_body_len);
                final_body[final_body_len] = '\0';
                ABE_Diag_RecordMemoryAlloc(final_body_len + 1);
            }
        }
    }

    kfree(rx_buf);

    memset(out_resp, 0, sizeof(ABE_HTTPResponse));
    out_resp->status_code = node->header_info.status_code;
    strncpy(out_resp->status_text, node->header_info.status_text, sizeof(out_resp->status_text) - 1);
    out_resp->header_count = node->header_info.header_count;
    memcpy(out_resp->headers, node->header_info.headers, sizeof(node->header_info.headers));
    out_resp->body_data = final_body;
    out_resp->body_len = final_body_len;
    out_resp->is_chunked = node->header_info.is_chunked;
    out_resp->is_gzipped = node->header_info.is_gzipped;
    out_resp->is_keep_alive = node->header_info.is_keep_alive;

    node->response = *out_resp;
    node->state = REQ_STATE_COMPLETED;

    ABE_Diag_RecordHTTPResponse(total_rcvd, 1200);
    ABE_LogVal(ABE_LOG_INFO, "NET", "Completed HTTP Transaction. Response Status: ", out_resp->status_code);
    return ABE_SUCCESS;
}

ABE_Error ABE_NetManager_CancelRequest(ABE_RequestHandle req) {
    if (!g_net_mgr.is_initialized || req == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_NetworkRequestNode* node = ABE_NetManager_GetNode(req);
    if (!node) return ABE_ERR_INVALID_PARAM;

    node->state = REQ_STATE_CANCELLED;
    if (g_net_mgr.active_request_count > 0) g_net_mgr.active_request_count--;
    ABE_LogVal(ABE_LOG_INFO, "NET", "Cancelled Network Request, Handle: ", req);
    return ABE_SUCCESS;
}

ABE_Error ABE_FreeHTTPResponse(ABE_HTTPResponse* resp) {
    if (!resp) return ABE_ERR_INVALID_PARAM;
    if (resp->body_data) {
        kfree(resp->body_data);
        ABE_Diag_RecordMemoryFree(resp->body_len + 1);
        resp->body_data = NULL;
    }
    resp->body_len = 0;
    return ABE_SUCCESS;
}

// Public SDK Networking Bridge Functions
ABE_Error ABE_NetworkInitialize(void) {
    return ABE_NetManager_Init();
}

ABE_Error ABE_NetworkShutdown(void) {
    return ABE_NetManager_Shutdown();
}

ABE_Error ABE_DNSResolve(const char* hostname, uint32_t* out_ip) {
    return ABE_NetDNS_Resolve(hostname, out_ip);
}

ABE_Error ABE_OpenConnection(const char* host, uint16_t port, bool use_tls, ABE_ConnHandle* out_conn) {
    return ABE_NetConn_Open(host, port, use_tls, out_conn);
}

ABE_Error ABE_CloseConnection(ABE_ConnHandle conn) {
    return ABE_NetConn_Close(conn);
}

ABE_Error ABE_SendHTTPRequest(ABE_ConnHandle conn, const ABE_HTTPRequest* req, ABE_RequestHandle* out_req) {
    return ABE_NetManager_SendRequest(conn, req, out_req);
}

ABE_Error ABE_ReadHTTPResponse(ABE_RequestHandle req, ABE_HTTPResponse* out_resp) {
    return ABE_NetManager_ReadResponse(req, out_resp);
}

ABE_Error ABE_DownloadResource(const char* url, ABE_DownloadProgressCallback cb, void* user_data, ABE_DownloadHandle* out_dl) {
    return ABE_NetDownload_Start(url, cb, user_data, out_dl);
}

ABE_Error ABE_CancelRequest(ABE_RequestHandle req) {
    return ABE_NetManager_CancelRequest(req);
}
