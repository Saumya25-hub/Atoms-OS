#include "abe_web_observers.h"
#include "../layout/abe_reflow.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

#define ABE_MAX_OBSERVERS 32

static ABE_WebObserverInstance g_observers[ABE_MAX_OBSERVERS];
static uint32_t g_next_obs_id = 2000;
static bool g_web_observers_initialized = false;

ABE_Error ABE_WebObservers_Init(void) {
    memset(g_observers, 0, sizeof(g_observers));
    g_web_observers_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WEBOBSERVERS", "ABE Web Observers Subsystem initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebObservers_Shutdown(void) {
    g_web_observers_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebObservers_CreateMutation(ABE_ObserverCallback cb, void* user_data, ABE_ObserverHandle* out_obs) {
    if (!g_web_observers_initialized || !cb || !out_obs) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_MAX_OBSERVERS; i++) {
        if (!g_observers[i].in_use) {
            ABE_WebObserverInstance* o = &g_observers[i];
            memset(o, 0, sizeof(ABE_WebObserverInstance));
            o->handle = (g_next_obs_id++) | (i << 16);
            o->type = OBSERVER_MUTATION;
            o->cb = cb;
            o->user_data = user_data;
            o->in_use = true;

            *out_obs = o->handle;
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_RESOURCE_EXHAUSTED;
}

ABE_Error ABE_WebObservers_ObserveNode(ABE_ObserverHandle obs, ABE_NodeHandle node) {
    if (!g_web_observers_initialized || obs == ABE_INVALID_HANDLE || node == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = (obs >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_OBSERVERS) return ABE_ERR_INVALID_PARAM;

    if (g_observers[slot].handle == obs && g_observers[slot].in_use) {
        g_observers[slot].target_node = node;
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_Error ABE_WebObservers_TriggerMutation(ABE_DocumentHandle doc, ABE_NodeHandle node) {
    if (!g_web_observers_initialized || node == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_MAX_OBSERVERS; i++) {
        if (g_observers[i].in_use && g_observers[i].target_node == node && g_observers[i].cb) {
            g_observers[i].cb(node, g_observers[i].user_data);
            ABE_Diag_RecordMutationObserverTrigger();
        }
    }

    ABE_RenderTreeHandle tree = ABE_INVALID_HANDLE;
    if (ABE_GetRenderTree(doc, &tree) == ABE_SUCCESS && tree != ABE_INVALID_HANDLE) {
        ABE_Reflow_MarkNodeDirty(tree, node);
        ABE_Reflow_Perform(tree);
    }
    return ABE_SUCCESS;
}
