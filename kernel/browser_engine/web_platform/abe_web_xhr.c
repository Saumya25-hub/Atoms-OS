#include "abe_web_xhr.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_XHRInstance g_xhr_pool[ABE_MAX_XHR_INSTANCES];
static uint32_t g_next_xhr_id = 3000;
static bool g_web_xhr_initialized = false;

ABE_Error ABE_WebXHR_Init(void) {
    memset(g_xhr_pool, 0, sizeof(g_xhr_pool));
    g_web_xhr_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WEBXHR", "ABE XMLHttpRequest Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebXHR_Shutdown(void) {
    g_web_xhr_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebXHR_Create(ABE_XHRHandle* out_xhr) {
    if (!g_web_xhr_initialized || !out_xhr) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_MAX_XHR_INSTANCES; i++) {
        if (!g_xhr_pool[i].in_use) {
            ABE_XHRInstance* x = &g_xhr_pool[i];
            memset(x, 0, sizeof(ABE_XHRInstance));
            x->handle = (g_next_xhr_id++) | (i << 16);
            x->state = XHR_STATE_UNSENT;
            x->in_use = true;

            *out_xhr = x->handle;
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_RESOURCE_EXHAUSTED;
}

ABE_Error ABE_WebXHR_Open(ABE_XHRHandle xhr, const char* method, const char* url) {
    if (!g_web_xhr_initialized || xhr == ABE_INVALID_HANDLE || !method || !url) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = (xhr >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_XHR_INSTANCES) return ABE_ERR_INVALID_PARAM;

    ABE_XHRInstance* x = &g_xhr_pool[slot];
    if (x->handle == xhr && x->in_use) {
        strncpy(x->method, method, sizeof(x->method) - 1);
        strncpy(x->url, url, sizeof(x->url) - 1);
        x->state = XHR_STATE_OPENED;
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_Error ABE_WebXHR_Send(ABE_XHRHandle xhr, const char* body) {
    if (!g_web_xhr_initialized || xhr == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = (xhr >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_XHR_INSTANCES) return ABE_ERR_INVALID_PARAM;

    ABE_XHRInstance* x = &g_xhr_pool[slot];
    if (x->handle == xhr && x->in_use && x->state == XHR_STATE_OPENED) {
        x->state = XHR_STATE_DONE;
        x->status = 200;
        strcpy(x->response_text, "OK");
        ABE_Diag_RecordXHRRequest();
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_Error ABE_WebXHR_Abort(ABE_XHRHandle xhr) {
    if (!g_web_xhr_initialized || xhr == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = (xhr >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_XHR_INSTANCES) return ABE_ERR_INVALID_PARAM;

    if (g_xhr_pool[slot].handle == xhr && g_xhr_pool[slot].in_use) {
        g_xhr_pool[slot].state = XHR_STATE_UNSENT;
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_PARAM;
}
