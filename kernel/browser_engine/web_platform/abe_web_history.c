#include "abe_web_history.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_WebHistory g_web_history;
static bool g_web_history_initialized = false;

ABE_Error ABE_WebHistory_Init(void) {
    memset(&g_web_history, 0, sizeof(ABE_WebHistory));
    g_web_history_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WEBHISTORY", "ABE History & Location Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebHistory_Shutdown(void) {
    g_web_history_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebHistory_PushState(const char* state_data, const char* title, const char* url) {
    if (!g_web_history_initialized || !url) return ABE_ERR_INVALID_PARAM;

    if (g_web_history.count < ABE_MAX_HISTORY_ENTRIES) {
        g_web_history.current_index = g_web_history.count;
        ABE_HistoryEntry* entry = &g_web_history.stack[g_web_history.count++];
        strncpy(entry->url, url, sizeof(entry->url) - 1);
        if (state_data) strncpy(entry->state_data, state_data, sizeof(entry->state_data) - 1);
        return ABE_SUCCESS;
    }
    return ABE_ERR_RESOURCE_EXHAUSTED;
}

ABE_Error ABE_WebHistory_ReplaceState(const char* state_data, const char* title, const char* url) {
    if (!g_web_history_initialized || !url) return ABE_ERR_INVALID_PARAM;

    if (g_web_history.count > 0) {
        ABE_HistoryEntry* entry = &g_web_history.stack[g_web_history.current_index];
        strncpy(entry->url, url, sizeof(entry->url) - 1);
        if (state_data) strncpy(entry->state_data, state_data, sizeof(entry->state_data) - 1);
        return ABE_SUCCESS;
    }
    return ABE_WebHistory_PushState(state_data, title, url);
}

ABE_Error ABE_WebHistory_Back(void) {
    if (!g_web_history_initialized) return ABE_ERR_NOT_INITIALIZED;
    if (g_web_history.current_index > 0) {
        g_web_history.current_index--;
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_Error ABE_WebHistory_Forward(void) {
    if (!g_web_history_initialized) return ABE_ERR_NOT_INITIALIZED;
    if (g_web_history.current_index + 1 < g_web_history.count) {
        g_web_history.current_index++;
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_PARAM;
}
