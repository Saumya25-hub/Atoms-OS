#include "abe_css_selector.h"
#include "../html/abe_html_element.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool g_selector_engine_initialized = false;

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

ABE_Error ABE_CSSSelector_Init(void) {
    g_selector_engine_initialized = true;
    ABE_Log(ABE_LOG_INFO, "SELECTOR", "ABE Selector Engine & Specificity Calculator initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_CSSSelector_Shutdown(void) {
    g_selector_engine_initialized = false;
    return ABE_SUCCESS;
}

int ABE_CSSSelector_CompareSpecificity(uint32_t a1, uint32_t b1, uint32_t c1, uint32_t a2, uint32_t b2, uint32_t c2) {
    if (a1 != a2) return (int)a1 - (int)a2;
    if (b1 != b2) return (int)b1 - (int)b2;
    return (int)c1 - (int)c2;
}

static bool MatchSingleSimpleSelector(const ABE_DOMNode* node, const char* sel) {
    if (!node || !sel || sel[0] == '\0') return false;
    if (node->type != ABE_NODE_ELEMENT) return false;

    if (sel[0] == '*') return true; // Universal selector

    if (sel[0] == '#') { // ID Selector
        const char* node_id = ABE_HTMLAttr_Get(node, "id");
        return (node_id && strcmp(node_id, sel + 1) == 0);
    }

    if (sel[0] == '.') { // Class Selector
        const char* node_cls = ABE_HTMLAttr_Get(node, "class");
        return (node_cls && strstr(node_cls, sel + 1) != NULL);
    }

    // Tag Element Selector
    return (StrCaseCmp(node->tag_name, sel) == 0);
}

bool ABE_CSSSelector_Match(const ABE_DOMNode* node, const char* selector_str) {
    if (!g_selector_engine_initialized || !node || !selector_str) return false;
    if (node->type != ABE_NODE_ELEMENT) return false;

    // Direct Single Selector Check
    if (strstr(selector_str, " ") == NULL && strstr(selector_str, ">") == NULL) {
        bool match = MatchSingleSimpleSelector(node, selector_str);
        if (match) ABE_Diag_RecordSelectorMatch();
        return match;
    }

    // Child Combinator "A > B"
    const char* child_op = strstr(selector_str, ">");
    if (child_op) {
        char parent_sel[64];
        char child_sel[64];

        size_t plen = (size_t)(child_op - selector_str);
        while (plen > 0 && selector_str[plen - 1] == ' ') plen--;
        strncpy(parent_sel, selector_str, plen);
        parent_sel[plen] = '\0';

        const char* cptr = child_op + 1;
        while (*cptr == ' ') cptr++;
        strncpy(child_sel, cptr, sizeof(child_sel) - 1);

        if (MatchSingleSimpleSelector(node, child_sel)) {
            if (node->parent && MatchSingleSimpleSelector(node->parent, parent_sel)) {
                ABE_Diag_RecordSelectorMatch();
                return true;
            }
        }
        return false;
    }

    // Descendant Combinator "A B"
    const char* space_op = strstr(selector_str, " ");
    if (space_op) {
        char ancestor_sel[64];
        char descendant_sel[64];

        size_t alen = (size_t)(space_op - selector_str);
        strncpy(ancestor_sel, selector_str, alen);
        ancestor_sel[alen] = '\0';

        const char* dptr = space_op + 1;
        while (*dptr == ' ') dptr++;
        strncpy(descendant_sel, dptr, sizeof(descendant_sel) - 1);

        if (MatchSingleSimpleSelector(node, descendant_sel)) {
            const ABE_DOMNode* curr = node->parent;
            while (curr) {
                if (MatchSingleSimpleSelector(curr, ancestor_sel)) {
                    ABE_Diag_RecordSelectorMatch();
                    return true;
                }
                curr = curr->parent;
            }
        }
        return false;
    }

    return false;
}
