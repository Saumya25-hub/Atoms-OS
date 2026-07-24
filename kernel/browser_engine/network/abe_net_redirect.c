#include "abe_net_redirect.h"
#include "../url/abe_url.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool g_redirect_initialized = false;

ABE_Error ABE_NetRedirect_Init(void) {
    g_redirect_initialized = true;
    ABE_Log(ABE_LOG_INFO, "REDIRECT", "ABE Production Redirect Engine V1.0 initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_NetRedirect_Shutdown(void) {
    g_redirect_initialized = false;
    return ABE_SUCCESS;
}

bool ABE_NetRedirect_IsRedirectCode(uint32_t status_code) {
    return (status_code == 301 || status_code == 302 || status_code == 303 || status_code == 307 || status_code == 308);
}

ABE_Error ABE_NetRedirect_ProcessRedirect(ABE_RedirectHistory* history, const char* current_url, uint32_t status_code, const char* location_header, ABE_HTTPMethod orig_method, char* out_new_url, ABE_HTTPMethod* out_new_method) {
    if (!g_redirect_initialized || !history || !current_url || !location_header || !out_new_url || !out_new_method) {
        return ABE_ERR_INVALID_PARAM;
    }

    if (!ABE_NetRedirect_IsRedirectCode(status_code)) {
        return ABE_ERR_INVALID_PARAM;
    }

    if (history->redirect_count >= ABE_MAX_REDIRECT_CHAIN) {
        ABE_Log(ABE_LOG_ERROR, "REDIRECT", "Redirect loop detected! Exceeded maximum redirect chain limit");
        return ABE_ERR_NET_TOO_MANY_REDIRECTS;
    }

    // Resolve relative or absolute Location header
    ABE_Error err = ABE_ResolveRelativeURL(current_url, location_header, out_new_url, ABE_MAX_URL_LEN);
    if (err != ABE_SUCCESS) return err;

    // Method Transformation Rules (RFC 9110)
    if (status_code == 303) {
        *out_new_method = ABE_HTTP_METHOD_GET;
    } else if (status_code == 301 || status_code == 302) {
        if (orig_method == ABE_HTTP_METHOD_POST) {
            *out_new_method = ABE_HTTP_METHOD_GET;
        } else {
            *out_new_method = orig_method;
        }
    } else { // 307 & 308 preserve method
        *out_new_method = orig_method;
    }

    ABE_RedirectEntry* entry = &history->chain[history->redirect_count++];
    strncpy(entry->source_url, current_url, ABE_MAX_URL_LEN - 1);
    strncpy(entry->target_url, out_new_url, ABE_MAX_URL_LEN - 1);
    entry->status_code = status_code;
    entry->timestamp = 1000;

    ABE_Diag_RecordRedirect();
    ABE_LogVal(ABE_LOG_INFO, "REDIRECT", "Redirecting (Code ", status_code);
    ABE_Log(ABE_LOG_INFO, "REDIRECT", out_new_url);
    return ABE_SUCCESS;
}
