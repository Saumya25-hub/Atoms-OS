#include "../../../sdk/include/abe/abe.h"
#include "abe_web_fetch.h"
#include "abe_web_xhr.h"
#include "abe_web_url.h"
#include "abe_web_storage.h"
#include "abe_web_history.h"
#include "abe_web_navigator.h"
#include "abe_web_performance.h"
#include "abe_web_scheduler.h"
#include "abe_web_observers.h"
#include "abe_web_blob.h"
#include "abe_web_form.h"
#include "abe_web_diag.h"
#include "../diagnostics/abe_diagnostics.h"

static bool g_web_api_initialized = false;

ABE_Error ABE_WebPlatformInitialize(void) {
    if (g_web_api_initialized) return ABE_ERR_ALREADY_INITIALIZED;

    ABE_WebFetch_Init();
    ABE_WebXHR_Init();
    ABE_WebURL_Init();
    ABE_WebStorage_Init();
    ABE_WebHistory_Init();
    ABE_WebNavigator_Init();
    ABE_WebPerformance_Init();
    ABE_WebScheduler_Init();
    ABE_WebObservers_Init();
    ABE_WebBlob_Init();
    ABE_WebForm_Init();
    ABE_WebDiag_Init();

    g_web_api_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WEBAPI", "ABE Web Platform Foundation Public SDK Bridge initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebPlatformShutdown(void) {
    if (!g_web_api_initialized) return ABE_ERR_NOT_INITIALIZED;

    ABE_WebDiag_Shutdown();
    ABE_WebForm_Shutdown();
    ABE_WebBlob_Shutdown();
    ABE_WebObservers_Shutdown();
    ABE_WebScheduler_Shutdown();
    ABE_WebPerformance_Shutdown();
    ABE_WebNavigator_Shutdown();
    ABE_WebHistory_Shutdown();
    ABE_WebStorage_Shutdown();
    ABE_WebURL_Shutdown();
    ABE_WebXHR_Shutdown();
    ABE_WebFetch_Shutdown();

    g_web_api_initialized = false;
    ABE_Log(ABE_LOG_INFO, "WEBAPI", "ABE Web Platform Foundation shut down cleanly");
    return ABE_SUCCESS;
}

ABE_Error ABE_Fetch(ABE_JSContextHandle ctx, const char* url, const ABE_HTTPRequest* init, ABE_FetchRequestHandle* out_req) {
    return ABE_WebFetch_Perform(ctx, url, init, out_req);
}

ABE_Error ABE_XMLHttpRequest_Create(ABE_XHRHandle* out_xhr) {
    return ABE_WebXHR_Create(out_xhr);
}

ABE_Error ABE_CreateURL(const char* url_str, ABE_URLHandle* out_url) {
    if (!url_str || !out_url) return ABE_ERR_INVALID_PARAM;
    *out_url = 101;
    return ABE_SUCCESS;
}

ABE_Error ABE_GetLocalStorage(ABE_StorageHandle* out_storage) {
    return ABE_WebStorage_GetLocal(out_storage);
}

ABE_Error ABE_GetSessionStorage(ABE_StorageHandle* out_storage) {
    return ABE_WebStorage_GetSession(out_storage);
}

ABE_Error ABE_Storage_SetItem(ABE_StorageHandle storage, const char* key, const char* val) {
    return ABE_WebStorage_Set(storage, key, val);
}

ABE_Error ABE_Storage_GetItem(ABE_StorageHandle storage, const char* key, char* out_buf, size_t max_len) {
    return ABE_WebStorage_Get(storage, key, out_buf, max_len);
}

ABE_Error ABE_RequestAnimationFrame(ABE_JSContextHandle ctx, void (*cb)(void*), void* user_data, uint32_t* out_id) {
    return ABE_WebScheduler_RequestAnimationFrame(ctx, cb, user_data, out_id);
}

ABE_Error ABE_CreateBlob(const uint8_t* data, size_t len, const char* mime_type, ABE_BlobHandle* out_blob) {
    return ABE_WebBlob_Create(data, len, mime_type, out_blob);
}

ABE_Error ABE_CreateMutationObserver(ABE_ObserverHandle* out_obs) {
    return ABE_WebObservers_CreateMutation(NULL, NULL, out_obs);
}
