#include "abe_css_style_manager.h"
#include "abe_css_parser.h"
#include "abe_css_selector.h"
#include "abe_css_cascade.h"
#include "abe_css_computed.h"
#include "../html/abe_html_document.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool g_style_mgr_initialized = false;

ABE_Error ABE_StyleManager_Init(void) {
    if (g_style_mgr_initialized) return ABE_ERR_ALREADY_INITIALIZED;

    ABE_CSSParser_Init();
    ABE_CSSSelector_Init();
    ABE_CSSCascade_Init();
    ABE_CSSComputed_Init();

    g_style_mgr_initialized = true;
    ABE_Log(ABE_LOG_INFO, "STYLEMGR", "ABE Style Manager initialized successfully");
    return ABE_SUCCESS;
}

ABE_Error ABE_StyleManager_Shutdown(void) {
    if (!g_style_mgr_initialized) return ABE_ERR_NOT_INITIALIZED;

    ABE_CSSComputed_Shutdown();
    ABE_CSSCascade_Shutdown();
    ABE_CSSSelector_Shutdown();
    ABE_CSSParser_Shutdown();

    g_style_mgr_initialized = false;
    ABE_Log(ABE_LOG_INFO, "STYLEMGR", "ABE Style Manager shut down cleanly");
    return ABE_SUCCESS;
}

ABE_Error ABE_StyleManager_LoadDocumentStyles(ABE_DocumentHandle doc_handle) {
    if (!g_style_mgr_initialized || doc_handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;

    ABE_DocumentStruct* doc = ABE_HTMLDoc_Get(doc_handle);
    if (!doc) return ABE_ERR_INVALID_PARAM;

    // Scan for internal <style> blocks
    ABE_NodeHandle style_nodes[16];
    uint32_t count = 0;
    if (ABE_FindElementsByTag(doc_handle, "style", style_nodes, 16, &count) == ABE_SUCCESS) {
        for (uint32_t i = 0; i < count; i++) {
            ABE_DOMNodeInfo info;
            if (ABE_GetNodeInfo(style_nodes[i], &info) == ABE_SUCCESS && info.first_child != ABE_INVALID_HANDLE) {
                ABE_DOMNodeInfo txt_info;
                if (ABE_GetNodeInfo(info.first_child, &txt_info) == ABE_SUCCESS) {
                    ABE_StylesheetHandle sheet_handle = ABE_INVALID_HANDLE;
                    if (ABE_CSSParser_ParseStylesheet(txt_info.node_value, strlen(txt_info.node_value), ORIGIN_AUTHOR, &sheet_handle) == ABE_SUCCESS) {
                        ABE_LogVal(ABE_LOG_INFO, "STYLEMGR", "Extracted internal <style> block, Handle: ", sheet_handle);
                    }
                }
            }
        }
    }

    // Compute styles for all DOM nodes in document
    return ABE_CSSComputed_ComputeDocumentStyles(doc_handle);
}
