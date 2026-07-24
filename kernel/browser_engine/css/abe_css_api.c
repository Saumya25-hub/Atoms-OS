#include "../../../sdk/include/abe/abe.h"
#include "abe_css_style_manager.h"
#include "abe_css_parser.h"
#include "abe_css_selector.h"
#include "abe_css_computed.h"
#include "../html/abe_dom_node.h"

ABE_Error ABE_CSSInitialize(void) {
    return ABE_StyleManager_Init();
}

ABE_Error ABE_CSSShutdown(void) {
    return ABE_StyleManager_Shutdown();
}

ABE_Error ABE_ParseStylesheet(const char* css_str, size_t len, ABE_StylesheetHandle* out_sheet) {
    return ABE_CSSParser_ParseStylesheet(css_str, len, ORIGIN_AUTHOR, out_sheet);
}

ABE_Error ABE_LoadStylesheet(ABE_DocumentHandle doc, ABE_StylesheetHandle sheet) {
    if (doc == ABE_INVALID_HANDLE || sheet == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    return ABE_CSSComputed_ComputeDocumentStyles(doc);
}

ABE_Error ABE_ComputeStyles(ABE_DocumentHandle doc) {
    return ABE_CSSComputed_ComputeDocumentStyles(doc);
}

ABE_Error ABE_GetComputedStyle(ABE_NodeHandle node, ABE_ComputedStyle* out_style) {
    return ABE_CSSComputed_GetNodeStyle(node, out_style);
}

ABE_Error ABE_MatchSelectors(ABE_NodeHandle node, const char* selector_str, bool* out_matched) {
    if (node == ABE_INVALID_HANDLE || !selector_str || !out_matched) return ABE_ERR_INVALID_PARAM;
    ABE_DOMNode* dom_node = ABE_DOM_GetNodeByHandle(node);
    if (!dom_node) return ABE_ERR_DOM_NODE_NOT_FOUND;

    *out_matched = ABE_CSSSelector_Match(dom_node, selector_str);
    return ABE_SUCCESS;
}

ABE_Error ABE_RecalculateStyles(ABE_DocumentHandle doc) {
    return ABE_CSSComputed_ComputeDocumentStyles(doc);
}

ABE_Error ABE_DestroyStylesheet(ABE_StylesheetHandle sheet) {
    return ABE_CSSParser_DestroyStylesheet(sheet);
}
