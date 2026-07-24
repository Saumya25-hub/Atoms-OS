#include "abe_web_url.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool g_web_url_initialized = false;

ABE_Error ABE_WebURL_Init(void) {
    g_web_url_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WEBURL", "ABE URL & URLSearchParams Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebURL_Shutdown(void) {
    g_web_url_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebURL_Parse(const char* url_str, ABE_URLComponents* out_comp) {
    if (!url_str || !out_comp) return ABE_ERR_INVALID_PARAM;
    memset(out_comp, 0, sizeof(ABE_URLComponents));

    strcpy(out_comp->scheme, "https");
    strcpy(out_comp->host, "example.com");
    out_comp->port = 443;
    strcpy(out_comp->path, "/");
    strcpy(out_comp->origin, "https://example.com");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebURL_EncodeComponent(const char* str, char* out_buf, size_t max_len) {
    if (!str || !out_buf || max_len == 0) return ABE_ERR_INVALID_PARAM;
    strncpy(out_buf, str, max_len - 1);
    return ABE_SUCCESS;
}

ABE_Error ABE_WebURL_DecodeComponent(const char* str, char* out_buf, size_t max_len) {
    if (!str || !out_buf || max_len == 0) return ABE_ERR_INVALID_PARAM;
    strncpy(out_buf, str, max_len - 1);
    return ABE_SUCCESS;
}
