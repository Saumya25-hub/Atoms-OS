#include "abe_net_http.h"
#include "../url/abe_url.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static bool g_http_initialized = false;

ABE_Error ABE_NetHTTP_Init(void) {
    g_http_initialized = true;
    ABE_Log(ABE_LOG_INFO, "HTTP", "ABE HTTP Request Engine & Serializer V1.0 initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_NetHTTP_Shutdown(void) {
    g_http_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_NetHTTP_CreateRequest(ABE_HTTPMethod method, const char* url_str, ABE_HTTPRequest* out_req) {
    if (!g_http_initialized || !url_str || !out_req) return ABE_ERR_INVALID_PARAM;
    memset(out_req, 0, sizeof(ABE_HTTPRequest));

    out_req->method = method;
    strncpy(out_req->url, url_str, ABE_MAX_URL_LEN - 1);

    ABE_URL parsed_url;
    ABE_Error err = ABE_ParseURL(url_str, &parsed_url);
    if (err != ABE_SUCCESS) return err;

    // Add Default Mandatory Headers
    ABE_NetHTTP_AddHeader(out_req, "Host", parsed_url.host);
    ABE_NetHTTP_AddHeader(out_req, "User-Agent", "Mozilla/5.0 (ATOMS OS; ABE/1.0)");
    ABE_NetHTTP_AddHeader(out_req, "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    ABE_NetHTTP_AddHeader(out_req, "Accept-Encoding", "gzip, deflate");
    ABE_NetHTTP_AddHeader(out_req, "Connection", "keep-alive");

    return ABE_SUCCESS;
}

ABE_Error ABE_NetHTTP_AddHeader(ABE_HTTPRequest* req, const char* name, const char* value) {
    if (!req || !name || !value) return ABE_ERR_INVALID_PARAM;
    if (req->header_count >= ABE_MAX_HEADERS) return ABE_ERR_RESOURCE_EXHAUSTED;

    for (uint32_t i = 0; i < req->header_count; i++) {
        if (strcmp(req->headers[i].name, name) == 0) {
            strncpy(req->headers[i].value, value, sizeof(req->headers[i].value) - 1);
            return ABE_SUCCESS;
        }
    }

    ABE_HTTPHeader* h = &req->headers[req->header_count++];
    strncpy(h->name, name, sizeof(h->name) - 1);
    strncpy(h->value, value, sizeof(h->value) - 1);
    return ABE_SUCCESS;
}

ABE_Error ABE_NetHTTP_SerializeRequest(const ABE_HTTPRequest* req, uint8_t** out_buf, size_t* out_len) {
    if (!req || !out_buf || !out_len) return ABE_ERR_INVALID_PARAM;

    ABE_URL parsed_url;
    ABE_Error err = ABE_ParseURL(req->url, &parsed_url);
    if (err != ABE_SUCCESS) return err;

    const char* method_str = "GET";
    switch (req->method) {
        case ABE_HTTP_METHOD_GET: method_str = "GET"; break;
        case ABE_HTTP_METHOD_HEAD: method_str = "HEAD"; break;
        case ABE_HTTP_METHOD_POST: method_str = "POST"; break;
        case ABE_HTTP_METHOD_OPTIONS: method_str = "OPTIONS"; break;
    }

    size_t alloc_bytes = 4096 + req->body_len;
    char* buf = (char*)kmalloc(alloc_bytes);
    if (!buf) return ABE_ERR_OUT_OF_MEMORY;
    memset(buf, 0, alloc_bytes);

    // 1. Request Line
    strncpy(buf, method_str, alloc_bytes - 1);
    strcat(buf, " ");
    strcat(buf, (parsed_url.path[0] != '\0') ? parsed_url.path : "/");
    if (parsed_url.query[0] != '\0') {
        strcat(buf, "?");
        strcat(buf, parsed_url.query);
    }
    strcat(buf, " HTTP/1.1\r\n");

    // 2. Headers
    for (uint32_t i = 0; i < req->header_count; i++) {
        strcat(buf, req->headers[i].name);
        strcat(buf, ": ");
        strcat(buf, req->headers[i].value);
        strcat(buf, "\r\n");
    }
    strcat(buf, "\r\n");

    size_t header_len = strlen(buf);

    // 3. Append Body Payload
    if (req->body_data && req->body_len > 0) {
        memcpy(buf + header_len, req->body_data, req->body_len);
    }

    size_t total_len = header_len + req->body_len;
    ABE_Diag_RecordMemoryAlloc(alloc_bytes);
    *out_buf = (uint8_t*)buf;
    *out_len = total_len;
    return ABE_SUCCESS;
}

void ABE_NetHTTP_FreeRequest(ABE_HTTPRequest* req) {
    if (!req) return;
    if (req->body_data) {
        kfree(req->body_data);
        req->body_data = NULL;
    }
    req->body_len = 0;
}
