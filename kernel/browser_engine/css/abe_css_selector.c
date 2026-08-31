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

static bool MatchClassList(const char* class_attr, const char* target_class) {
    if (!class_attr || !target_class) return false;
    size_t target_len = strlen(target_class);
    if (target_len == 0) return false;

    const char* p = class_attr;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;

        const char* start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        size_t len = (size_t)(p - start);

        if (len == target_len && strncmp(start, target_class, len) == 0) {
            return true;
        }
    }
    return false;
}

static bool MatchCompoundSimpleSelector(const ABE_DOMNode* node, const char* sel) {
    if (!node || !sel || sel[0] == '\0') return false;
    if (node->type != ABE_NODE_ELEMENT) return false;

    if (strcmp(sel, "*") == 0) return true;

    const char* p = sel;
    char tag_buf[64] = {0};
    size_t tag_idx = 0;

    // Parse leading tag name (if any)
    while (*p && *p != '.' && *p != '#' && *p != '[' && *p != ':') {
        if (tag_idx < sizeof(tag_buf) - 1) tag_buf[tag_idx++] = *p;
        p++;
    }
    tag_buf[tag_idx] = '\0';

    if (tag_idx > 0 && strcmp(tag_buf, "*") != 0) {
        if (StrCaseCmp(node->tag_name, tag_buf) != 0) return false;
    }

    // Process qualifiers (.class, #id, [attr], :pseudo)
    while (*p) {
        if (*p == '#') {
            p++;
            char id_buf[64] = {0};
            size_t id_idx = 0;
            while (*p && *p != '.' && *p != '#' && *p != '[' && *p != ':') {
                if (id_idx < sizeof(id_buf) - 1) id_buf[id_idx++] = *p;
                p++;
            }
            id_buf[id_idx] = '\0';
            const char* node_id = ABE_HTMLAttr_Get(node, "id");
            if (!node_id || strcmp(node_id, id_buf) != 0) return false;
        } else if (*p == '.') {
            p++;
            char cls_buf[64] = {0};
            size_t cls_idx = 0;
            while (*p && *p != '.' && *p != '#' && *p != '[' && *p != ':') {
                if (cls_idx < sizeof(cls_buf) - 1) cls_buf[cls_idx++] = *p;
                p++;
            }
            cls_buf[cls_idx] = '\0';
            const char* node_cls = ABE_HTMLAttr_Get(node, "class");
            if (!node_cls || !MatchClassList(node_cls, cls_buf)) return false;
        } else if (*p == '[') {
            p++;
            char attr_name[64] = {0};
            char attr_val[64] = {0};
            size_t an_idx = 0, av_idx = 0;
            char op = 0;

            while (*p && *p != '=' && *p != '~' && *p != ']') {
                if (an_idx < sizeof(attr_name) - 1) attr_name[an_idx++] = *p;
                p++;
            }
            attr_name[an_idx] = '\0';

            if (*p == '~' && *(p + 1) == '=') { op = '~'; p += 2; }
            else if (*p == '=') { op = '='; p++; }

            if (op != 0) {
                if (*p == '"' || *p == '\'') p++; // skip quote
                while (*p && *p != '"' && *p != '\'' && *p != ']') {
                    if (av_idx < sizeof(attr_val) - 1) attr_val[av_idx++] = *p;
                    p++;
                }
                if (*p == '"' || *p == '\'') p++;
            }
            while (*p && *p != ']') p++;
            if (*p == ']') p++;
            attr_val[av_idx] = '\0';

            const char* node_attr = ABE_HTMLAttr_Get(node, attr_name);
            if (!node_attr) return false;
            if (op == '=' && strcmp(node_attr, attr_val) != 0) return false;
            if (op == '~' && !MatchClassList(node_attr, attr_val)) return false;
        } else if (*p == ':') {
            p++;
            if (*p == ':') p++; // pseudo-element
            char pseudo_name[64] = {0};
            size_t ps_idx = 0;
            while (*p && *p != '.' && *p != '#' && *p != '[' && *p != ':') {
                if (ps_idx < sizeof(pseudo_name) - 1) pseudo_name[ps_idx++] = *p;
                p++;
            }
            pseudo_name[ps_idx] = '\0';

            if (StrCaseCmp(pseudo_name, "first-child") == 0) {
                if (node->prev_sibling != NULL) return false;
            } else if (StrCaseCmp(pseudo_name, "last-child") == 0) {
                if (node->next_sibling != NULL) return false;
            } else if (StrCaseCmp(pseudo_name, "disabled") == 0) {
                if (!ABE_HTMLAttr_Has(node, "disabled")) return false;
            } else if (StrCaseCmp(pseudo_name, "checked") == 0) {
                if (!ABE_HTMLAttr_Has(node, "checked")) return false;
            }
            // For :hover, :focus, :active -> match allowed representation
        } else {
            p++;
        }
    }

    return true;
}

static bool MatchSingleSelectorChain(const ABE_DOMNode* node, const char* sel_str) {
    if (!node || !sel_str || sel_str[0] == '\0') return false;

    // Adjacent Sibling Combinator "A + B"
    const char* adj_op = strstr(sel_str, "+");
    if (adj_op) {
        char left_sel[64];
        char right_sel[64];

        size_t llen = (size_t)(adj_op - sel_str);
        while (llen > 0 && sel_str[llen - 1] == ' ') llen--;
        strncpy(left_sel, sel_str, llen);
        left_sel[llen] = '\0';

        const char* rptr = adj_op + 1;
        while (*rptr == ' ') rptr++;
        strncpy(right_sel, rptr, sizeof(right_sel) - 1);

        if (MatchCompoundSimpleSelector(node, right_sel)) {
            if (node->prev_sibling && MatchCompoundSimpleSelector(node->prev_sibling, left_sel)) {
                return true;
            }
        }
        return false;
    }

    // General Sibling Combinator "A ~ B"
    const char* sib_op = strstr(sel_str, "~");
    if (sib_op) {
        char left_sel[64];
        char right_sel[64];

        size_t llen = (size_t)(sib_op - sel_str);
        while (llen > 0 && sel_str[llen - 1] == ' ') llen--;
        strncpy(left_sel, sel_str, llen);
        left_sel[llen] = '\0';

        const char* rptr = sib_op + 1;
        while (*rptr == ' ') rptr++;
        strncpy(right_sel, rptr, sizeof(right_sel) - 1);

        if (MatchCompoundSimpleSelector(node, right_sel)) {
            const ABE_DOMNode* prev = node->prev_sibling;
            while (prev) {
                if (MatchCompoundSimpleSelector(prev, left_sel)) return true;
                prev = prev->prev_sibling;
            }
        }
        return false;
    }

    // Child Combinator "A > B"
    const char* child_op = strstr(sel_str, ">");
    if (child_op) {
        char parent_sel[64];
        char child_sel[64];

        size_t plen = (size_t)(child_op - sel_str);
        while (plen > 0 && sel_str[plen - 1] == ' ') plen--;
        strncpy(parent_sel, sel_str, plen);
        parent_sel[plen] = '\0';

        const char* cptr = child_op + 1;
        while (*cptr == ' ') cptr++;
        strncpy(child_sel, cptr, sizeof(child_sel) - 1);

        if (MatchCompoundSimpleSelector(node, child_sel)) {
            if (node->parent && MatchCompoundSimpleSelector(node->parent, parent_sel)) {
                return true;
            }
        }
        return false;
    }

    // Descendant Combinator "A B"
    const char* space_op = strstr(sel_str, " ");
    if (space_op) {
        char ancestor_sel[64];
        char descendant_sel[64];

        size_t alen = (size_t)(space_op - sel_str);
        strncpy(ancestor_sel, sel_str, alen);
        ancestor_sel[alen] = '\0';

        const char* dptr = space_op + 1;
        while (*dptr == ' ') dptr++;
        strncpy(descendant_sel, dptr, sizeof(descendant_sel) - 1);

        if (MatchCompoundSimpleSelector(node, descendant_sel)) {
            const ABE_DOMNode* curr = node->parent;
            while (curr) {
                if (MatchCompoundSimpleSelector(curr, ancestor_sel)) {
                    return true;
                }
                curr = curr->parent;
            }
        }
        return false;
    }

    return MatchCompoundSimpleSelector(node, sel_str);
}

bool ABE_CSSSelector_Match(const ABE_DOMNode* node, const char* selector_str) {
    if (!g_selector_engine_initialized || !node || !selector_str) return false;
    if (node->type != ABE_NODE_ELEMENT) return false;

    // Handle comma-separated selector lists "h1, h2, h3"
    const char* p = selector_str;
    while (*p) {
        while (*p == ' ' || *p == ',') p++;
        if (!*p) break;

        char sub_sel[128];
        size_t idx = 0;
        while (*p && *p != ',') {
            if (idx < sizeof(sub_sel) - 1) sub_sel[idx++] = *p;
            p++;
        }
        while (idx > 0 && sub_sel[idx - 1] == ' ') idx--;
        sub_sel[idx] = '\0';

        if (MatchSingleSelectorChain(node, sub_sel)) {
            ABE_Diag_RecordSelectorMatch();
            return true;
        }
        if (*p == ',') p++;
    }

    return false;
}

