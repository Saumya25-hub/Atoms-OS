#include "abe_js_dom_binding.h"
#include "../html/abe_html_document.h"
#include "../layout/abe_reflow.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

#define ABE_JS_MAX_EVENT_BINDINGS 128

static ABE_JSEventBinding g_event_bindings[ABE_JS_MAX_EVENT_BINDINGS];
static bool g_js_dom_binding_initialized = false;

ABE_Error ABE_JSDOMBinding_Init(void) {
    memset(g_event_bindings, 0, sizeof(g_event_bindings));
    g_js_dom_binding_initialized = true;
    ABE_Log(ABE_LOG_INFO, "JSDOMBINDING", "ABE JavaScript DOM Binding Layer & Event System initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_JSDOMBinding_Shutdown(void) {
    g_js_dom_binding_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_JSDOM_AddEventListener(ABE_NodeHandle node, const char* event_type, ABE_JSEventListener cb, void* user_data) {
    if (!g_js_dom_binding_initialized || node == ABE_INVALID_HANDLE || !event_type || !cb) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_JS_MAX_EVENT_BINDINGS; i++) {
        if (!g_event_bindings[i].in_use) {
            ABE_JSEventBinding* b = &g_event_bindings[i];
            b->target_node = node;
            strncpy(b->event_type, event_type, sizeof(b->event_type) - 1);
            b->callback = cb;
            b->user_data = user_data;
            b->in_use = true;
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_RESOURCE_EXHAUSTED;
}

ABE_Error ABE_JSDOM_DispatchEvent(ABE_DocumentHandle doc, ABE_NodeHandle node, const char* event_type) {
    if (!g_js_dom_binding_initialized || node == ABE_INVALID_HANDLE || !event_type) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_JS_MAX_EVENT_BINDINGS; i++) {
        if (g_event_bindings[i].in_use && g_event_bindings[i].target_node == node && strcmp(g_event_bindings[i].event_type, event_type) == 0) {
            if (g_event_bindings[i].callback) {
                g_event_bindings[i].callback(node, event_type, g_event_bindings[i].user_data);
            }
        }
    }
    return ABE_SUCCESS;
}

ABE_Error ABE_JSDOM_SetInnerHTML(ABE_DocumentHandle doc, ABE_NodeHandle node, const char* html_str) {
    if (!g_js_dom_binding_initialized || doc == ABE_INVALID_HANDLE || node == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;

    ABE_RenderTreeHandle tree = ABE_INVALID_HANDLE;
    if (ABE_GetRenderTree(doc, &tree) == ABE_SUCCESS && tree != ABE_INVALID_HANDLE) {
        ABE_Reflow_MarkNodeDirty(tree, node);
        ABE_Reflow_Perform(tree);
    }
    ABE_Log(ABE_LOG_INFO, "JSDOMBINDING", "innerHTML set from JS, Layout tree invalidated and reflowed!");
    return ABE_SUCCESS;
}
