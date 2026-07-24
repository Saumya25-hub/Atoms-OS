#include "abe_css_computed.h"
#include "abe_css_parser.h"
#include "abe_css_selector.h"
#include "abe_css_cascade.h"
#include "abe_css_inherit.h"
#include "../html/abe_html_document.h"
#include "../html/abe_html_element.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

#define ABE_MAX_COMPUTED_NODES 1024

typedef struct {
    ABE_NodeHandle handle;
    ABE_ComputedStyle style;
    bool valid;
} ABE_ComputedNodeStore;

static ABE_ComputedNodeStore g_computed_store[ABE_MAX_COMPUTED_NODES];
static bool g_computed_engine_initialized = false;

static char ToLowerChar(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

static int StrCaseCmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return 1;
    while (*s1 && *s2) {
        char c1 = ToLowerChar(*s1);
        char c2 = ToLowerChar(*s2);
        if (c1 != c2) return 1;
        s1++; s2++;
    }
    return (*s1 == '\0' && *s2 == '\0') ? 0 : 1;
}

ABE_Error ABE_CSSComputed_Init(void) {
    memset(g_computed_store, 0, sizeof(g_computed_store));
    g_computed_engine_initialized = true;
    ABE_Log(ABE_LOG_INFO, "COMPUTED", "ABE Computed Style Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_CSSComputed_Shutdown(void) {
    if (!g_computed_engine_initialized) return ABE_ERR_NOT_INITIALIZED;
    memset(g_computed_store, 0, sizeof(g_computed_store));
    g_computed_engine_initialized = false;
    return ABE_SUCCESS;
}

static void GetDefaultComputedStyle(ABE_ComputedStyle* style) {
    if (!style) return;
    memset(style, 0, sizeof(ABE_ComputedStyle));

    style->display = ABE_DISPLAY_INLINE;
    style->position = ABE_POSITION_STATIC;

    style->width_auto = true;
    style->height_auto = true;

    style->color = 0xFF000000;            // Black
    style->background_color = 0x00000000; // Transparent

    style->font_size_px = 16.0f;
    style->font_weight = 400;
    style->line_height_px = 20.0f;
    style->text_align = ABE_TEXT_ALIGN_LEFT;
    strncpy(style->font_family, "sans-serif", sizeof(style->font_family) - 1);

    style->opacity = 1.0f;
    style->visibility = ABE_VISIBILITY_VISIBLE;
    style->overflow = ABE_OVERFLOW_VISIBLE;
}

static void ApplyDeclarationToStyle(const ABE_CSSDeclaration* decl, ABE_ComputedStyle* style) {
    if (!decl || !style) return;

    if (StrCaseCmp(decl->name, "display") == 0) {
        if (StrCaseCmp(decl->value.str_val, "block") == 0) style->display = ABE_DISPLAY_BLOCK;
        else if (StrCaseCmp(decl->value.str_val, "inline") == 0) style->display = ABE_DISPLAY_INLINE;
        else if (StrCaseCmp(decl->value.str_val, "inline-block") == 0) style->display = ABE_DISPLAY_INLINE_BLOCK;
        else if (StrCaseCmp(decl->value.str_val, "none") == 0) style->display = ABE_DISPLAY_NONE;
        else if (StrCaseCmp(decl->value.str_val, "flex") == 0) style->display = ABE_DISPLAY_FLEX;
        else if (StrCaseCmp(decl->value.str_val, "table") == 0) style->display = ABE_DISPLAY_TABLE;
    } else if (StrCaseCmp(decl->name, "position") == 0) {
        if (StrCaseCmp(decl->value.str_val, "relative") == 0) style->position = ABE_POSITION_RELATIVE;
        else if (StrCaseCmp(decl->value.str_val, "absolute") == 0) style->position = ABE_POSITION_ABSOLUTE;
        else if (StrCaseCmp(decl->value.str_val, "fixed") == 0) style->position = ABE_POSITION_FIXED;
    } else if (StrCaseCmp(decl->name, "width") == 0) {
        if (decl->value.type == CSS_VAL_AUTO) {
            style->width_auto = true;
        } else {
            style->width_auto = false;
            style->width_px = decl->value.number_val;
        }
    } else if (StrCaseCmp(decl->name, "height") == 0) {
        if (decl->value.type == CSS_VAL_AUTO) {
            style->height_auto = true;
        } else {
            style->height_auto = false;
            style->height_px = decl->value.number_val;
        }
    } else if (StrCaseCmp(decl->name, "margin") == 0) {
        style->margin_top_px = decl->value.number_val;
        style->margin_right_px = decl->value.number_val;
        style->margin_bottom_px = decl->value.number_val;
        style->margin_left_px = decl->value.number_val;
    } else if (StrCaseCmp(decl->name, "padding") == 0) {
        style->padding_top_px = decl->value.number_val;
        style->padding_right_px = decl->value.number_val;
        style->padding_bottom_px = decl->value.number_val;
        style->padding_left_px = decl->value.number_val;
    } else if (StrCaseCmp(decl->name, "color") == 0) {
        if (decl->value.type == CSS_VAL_COLOR) {
            style->color = decl->value.color_val;
        }
    } else if (StrCaseCmp(decl->name, "background-color") == 0) {
        if (decl->value.type == CSS_VAL_COLOR) {
            style->background_color = decl->value.color_val;
        }
    } else if (StrCaseCmp(decl->name, "font-size") == 0) {
        if (decl->value.type == CSS_VAL_PX) {
            style->font_size_px = decl->value.number_val;
        }
    } else if (StrCaseCmp(decl->name, "font-weight") == 0) {
        if (decl->value.type == CSS_VAL_PX) {
            style->font_weight = (uint32_t)decl->value.number_val;
        }
    }
}

static void ComputeNodeStyleRecursive(ABE_DOMNode* node, const ABE_ComputedStyle* parent_style) {
    if (!node) return;

    uint32_t slot = (node->handle >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_COMPUTED_NODES) return;

    ABE_ComputedNodeStore* store = &g_computed_store[slot];
    store->handle = node->handle;
    GetDefaultComputedStyle(&store->style);

    // 1. Apply Inheritance from Parent
    if (parent_style) {
        ABE_CSSInherit_PropagateInheritedStyles(parent_style, &store->style);
    }

    // 2. Cascade User-Agent Stylesheet
    ABE_StylesheetHandle ua_sheet_handle = ABE_CSSCascade_GetUserAgentStylesheet();
    ABE_CSSStylesheet* ua_sheet = ABE_CSSParser_GetStylesheet(ua_sheet_handle);
    if (ua_sheet) {
        for (uint32_t i = 0; i < ua_sheet->rule_count; i++) {
            ABE_CSSRule* rule = &ua_sheet->rules[i];
            if (ABE_CSSSelector_Match(node, rule->selector_str)) {
                for (uint32_t j = 0; j < rule->declaration_count; j++) {
                    ApplyDeclarationToStyle(&rule->declarations[j], &store->style);
                }
            }
        }
    }

    // 3. Cascade Inline Style Attribute `style="..."`
    const char* inline_style_attr = ABE_HTMLAttr_Get(node, "style");
    if (inline_style_attr && inline_style_attr[0] != '\0') {
        ABE_StylesheetHandle inline_handle = ABE_INVALID_HANDLE;
        char inline_wrapper[512];
        strncpy(inline_wrapper, "node { ", sizeof(inline_wrapper) - 1);
        strcat(inline_wrapper, inline_style_attr);
        strcat(inline_wrapper, " }");

        if (ABE_CSSParser_ParseStylesheet(inline_wrapper, strlen(inline_wrapper), ORIGIN_INLINE, &inline_handle) == ABE_SUCCESS) {
            ABE_CSSStylesheet* inline_sheet = ABE_CSSParser_GetStylesheet(inline_handle);
            if (inline_sheet && inline_sheet->rule_count > 0) {
                for (uint32_t j = 0; j < inline_sheet->rules[0].declaration_count; j++) {
                    ApplyDeclarationToStyle(&inline_sheet->rules[0].declarations[j], &store->style);
                }
            }
            ABE_CSSParser_DestroyStylesheet(inline_handle);
        }
    }

    store->valid = true;
    ABE_Diag_RecordStyleComputed(12);

    // Recurse to children
    ABE_DOMNode* child = node->first_child;
    while (child) {
        ComputeNodeStyleRecursive(child, &store->style);
        child = child->next_sibling;
    }
}

ABE_Error ABE_CSSComputed_ComputeDocumentStyles(ABE_DocumentHandle doc_handle) {
    if (!g_computed_engine_initialized || doc_handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_DocumentStruct* doc = ABE_HTMLDoc_Get(doc_handle);
    if (!doc || !doc->root_node) return ABE_ERR_INVALID_PARAM;

    ComputeNodeStyleRecursive(doc->root_node, NULL);
    ABE_Log(ABE_LOG_INFO, "COMPUTED", "Computed Styles successfully generated for entire DOM tree!");
    return ABE_SUCCESS;
}

ABE_Error ABE_CSSComputed_GetNodeStyle(ABE_NodeHandle node_handle, ABE_ComputedStyle* out_style) {
    if (!g_computed_engine_initialized || node_handle == ABE_INVALID_HANDLE || !out_style) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = (node_handle >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_COMPUTED_NODES) return ABE_ERR_DOM_NODE_NOT_FOUND;

    ABE_ComputedNodeStore* store = &g_computed_store[slot];
    if (store->handle != node_handle || !store->valid) return ABE_ERR_DOM_NODE_NOT_FOUND;

    *out_style = store->style;
    return ABE_SUCCESS;
}
