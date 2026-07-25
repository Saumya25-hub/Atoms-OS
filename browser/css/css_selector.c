#include "css_selector.h"
#include "css_specificity.h"
#include "browser/html/attributes/html_attribute.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static bool str_eq_ci(const char* s1, const char* s2) {
    if (!s1 || !s2) return false;
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return false;
        s1++;
        s2++;
    }
    return (*s1 == '\0' && *s2 == '\0');
}

css_status_t css_selector_create(css_selector_t** out_selector) {
    if (!out_selector) return CSS_ERR_INVALID_PARAM;
    css_selector_t* sel = (css_selector_t*)kmalloc(sizeof(css_selector_t));
    if (!sel) return CSS_ERR_OUT_OF_MEMORY;
    memset(sel, 0, sizeof(css_selector_t));
    *out_selector = sel;
    return CSS_OK;
}

void css_selector_destroy(css_selector_t* selector) {
    while (selector) {
        css_selector_t* next_group = selector->next_group;

        css_selector_item_t* item = selector->items_head;
        while (item) {
            css_selector_item_t* next_item = item->next;
            kfree(item);
            item = next_item;
        }

        kfree(selector);
        selector = next_group;
    }
}

static css_selector_item_t* css_selector_item_create(css_selector_type_t type, const char* val) {
    css_selector_item_t* item = (css_selector_item_t*)kmalloc(sizeof(css_selector_item_t));
    if (!item) return NULL;
    memset(item, 0, sizeof(css_selector_item_t));
    item->type = type;
    if (val) {
        size_t k = 0;
        while (k < sizeof(item->value) - 1 && val[k] != '\0') {
            item->value[k] = val[k];
            k++;
        }
        item->value[k] = '\0';
    }
    return item;
}

css_status_t css_selector_parse_string(const char* raw_str, css_selector_t** out_selector) {
    if (!raw_str || !out_selector) return CSS_ERR_INVALID_PARAM;
    *out_selector = NULL;

    css_selector_t* head_group = NULL;
    css_selector_t* tail_group = NULL;

    const char* p = raw_str;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;

        css_selector_t* current_sel = NULL;
        if (css_selector_create(&current_sel) != CSS_OK) break;

        css_selector_item_t* tail_item = NULL;

        while (*p && *p != ',') {
            bool had_space = false;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
                had_space = true;
                p++;
            }
            if (!*p || *p == ',' || *p == '{') break;

            css_combinator_t comb = CSS_COMB_NONE;

            if (*p == '>') { comb = CSS_COMB_CHILD; p++; while (*p == ' ') p++; }
            else if (*p == '+') { comb = CSS_COMB_ADJACENT_SIBLING; p++; while (*p == ' ') p++; }
            else if (*p == '~') { comb = CSS_COMB_GENERAL_SIBLING; p++; while (*p == ' ') p++; }
            else if (had_space && tail_item != NULL) { comb = CSS_COMB_DESCENDANT; }

            css_selector_type_t stype = CSS_SEL_TAG;
            char name_buf[64];
            size_t n_idx = 0;

            if (*p == '#') {
                stype = CSS_SEL_ID;
                p++;
            } else if (*p == '.') {
                stype = CSS_SEL_CLASS;
                p++;
            } else if (*p == '*') {
                stype = CSS_SEL_UNIVERSAL;
                name_buf[n_idx++] = '*';
                p++;
            }

            while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' &&
                   *p != ',' && *p != '{' && *p != '>' && *p != '+' && *p != '~' &&
                   *p != '#' && *p != '.') {
                if (n_idx < sizeof(name_buf) - 1) name_buf[n_idx++] = *p;
                p++;
            }
            name_buf[n_idx] = '\0';

            css_selector_item_t* item = css_selector_item_create(stype, name_buf);
            if (!item) break;

            if (tail_item) {
                tail_item->combinator = comb;
                tail_item->next = item;
                tail_item = item;
            } else {
                current_sel->items_head = item;
                tail_item = item;
            }
        }

        css_specificity_calculate(current_sel);

        if (!head_group) {
            head_group = current_sel;
            tail_group = current_sel;
        } else {
            tail_group->next_group = current_sel;
            tail_group = current_sel;
        }

        if (*p == ',') p++;
    }

    *out_selector = head_group;
    return (head_group != NULL) ? CSS_OK : CSS_ERR_PARSE_ERROR;
}

static bool match_single_item(const css_selector_item_t* item, const bos_node_t* node) {
    if (!item || !node) return false;
    if (node->type != BOS_NODE_ELEMENT) return false;

    if (item->type == CSS_SEL_UNIVERSAL) return true;

    if (item->type == CSS_SEL_TAG) {
        return str_eq_ci(item->value, node->name);
    }

    if (item->type == CSS_SEL_ID) {
        const char* id_val = html_attr_get(node, "id");
        return id_val && str_eq_ci(id_val, item->value);
    }

    if (item->type == CSS_SEL_CLASS) {
        const char* class_val = html_attr_get(node, "class");
        if (!class_val) return false;

        // Check space separated classes
        const char* c = class_val;
        while (*c) {
            while (*c == ' ') c++;
            if (!*c) break;
            const char* start = c;
            while (*c && *c != ' ') c++;
            size_t len = c - start;
            if (strlen(item->value) == len && strncmp(start, item->value, len) == 0) {
                return true;
            }
        }
        return false;
    }

    return false;
}

static bool match_chain(const css_selector_item_t* item, const bos_node_t* node) {
    if (!item) return true;
    if (!node) return false;

    if (!match_single_item(item, node)) return false;

    if (!item->next) return true;

    css_combinator_t comb = item->combinator;
    const css_selector_item_t* next_item = item->next;

    if (comb == CSS_COMB_NONE) {
        return match_chain(next_item, node);
    }

    if (comb == CSS_COMB_CHILD) {
        return node->parent && match_chain(next_item, node->parent);
    }


    if (comb == CSS_COMB_DESCENDANT) {
        const bos_node_t* parent = node->parent;
        while (parent) {
            if (match_chain(next_item, parent)) return true;
            parent = parent->parent;
        }
        return false;
    }

    if (comb == CSS_COMB_ADJACENT_SIBLING) {
        return node->previous_sibling && match_chain(next_item, node->previous_sibling);
    }

    if (comb == CSS_COMB_GENERAL_SIBLING) {
        const bos_node_t* sib = node->previous_sibling;
        while (sib) {
            if (match_chain(next_item, sib)) return true;
            sib = sib->previous_sibling;
        }
        return false;
    }

    return false;
}

bool css_selector_match_node(const css_selector_t* selector, const bos_node_t* node) {
    if (!selector || !node) return false;

    const css_selector_t* group = selector;
    while (group) {
        if (group->items_head && match_chain(group->items_head, node)) {
            return true;
        }
        group = group->next_group;
    }
    return false;
}
