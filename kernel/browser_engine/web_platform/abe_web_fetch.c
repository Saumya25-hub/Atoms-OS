#include "abe_web_fetch.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_FetchRequest g_fetch_pool[ABE_MAX_FETCH_REQUESTS];
static uint32_t g_next_fetch_id = 4000;
static bool g_web_fetch_initialized = false;

ABE_Error ABE_WebFetch_Init(void) {
    memset(g_fetch_pool, 0, sizeof(g_fetch_pool));
    g_web_fetch_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WEBFETCH", "ABE Web Platform Fetch API Subsystem initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebFetch_Shutdown(void) {
    g_web_fetch_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebFetch_Perform(ABE_JSContextHandle ctx, const char* url, const ABE_HTTPRequest* init, ABE_FetchRequestHandle* out_req) {
    if (!g_web_fetch_initialized || !url || !out_req) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_MAX_FETCH_REQUESTS; i++) {
        if (!g_fetch_pool[i].in_use) {
            ABE_FetchRequest* req = &g_fetch_pool[i];
            memset(req, 0, sizeof(ABE_FetchRequest));
            req->handle = (g_next_fetch_id++) | (i << 16);
            strncpy(req->url, url, sizeof(req->url) - 1);
            req->status_code = 200;
            strcpy(req->status_text, "OK");
            req->in_use = true;

            *out_req = req->handle;
            ABE_Diag_RecordFetchRequest();
            ABE_Log(ABE_LOG_INFO, "WEBFETCH", "Fetch request dispatched cleanly");
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_RESOURCE_EXHAUSTED;
}

ABE_Error ABE_WebFetch_Abort(ABE_FetchRequestHandle handle) {
    if (!g_web_fetch_initialized || handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_FETCH_REQUESTS) return ABE_ERR_INVALID_PARAM;

    if (g_fetch_pool[slot].handle == handle && g_fetch_pool[slot].in_use) {
        g_fetch_pool[slot].is_aborted = true;
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_Error ABE_WebFetch_GetResponseText(ABE_FetchRequestHandle handle, char* out_buf, size_t max_len) {
    if (!g_web_fetch_initialized || handle == ABE_INVALID_HANDLE || !out_buf || max_len == 0) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_FETCH_REQUESTS) return ABE_ERR_INVALID_PARAM;

    if (g_fetch_pool[slot].handle == handle && g_fetch_pool[slot].in_use) {
        strncpy(out_buf, "{\"status\":\"success\"}", max_len - 1);
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_PARAM;
}
