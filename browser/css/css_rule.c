#include "css_rule.h"
#include "css_selector.h"
#include "css_value.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

css_status_t css_rule_create(css_rule_type_t type, css_rule_t** out_rule) {
    if (!out_rule) return CSS_ERR_INVALID_PARAM;
    css_rule_t* rule = (css_rule_t*)kmalloc(sizeof(css_rule_t));
    if (!rule) return CSS_ERR_OUT_OF_MEMORY;
    memset(rule, 0, sizeof(css_rule_t));
    rule->type = type;
    *out_rule = rule;
    return CSS_OK;
}

void css_rule_destroy(css_rule_t* rule) {
    while (rule) {
        css_rule_t* next_rule = rule->next;

        if (rule->selectors) {
            css_selector_destroy(rule->selectors);
        }

        css_declaration_t* decl = rule->declarations;
        while (decl) {
            css_declaration_t* next_decl = decl->next;
            css_declaration_destroy(decl);
            decl = next_decl;
        }

        kfree(rule);
        rule = next_rule;
    }
}

css_status_t css_declaration_create(const char* name, const char* val_str, css_declaration_t** out_decl) {
    if (!name || !val_str || !out_decl) return CSS_ERR_INVALID_PARAM;

    css_declaration_t* decl = (css_declaration_t*)kmalloc(sizeof(css_declaration_t));
    if (!decl) return CSS_ERR_OUT_OF_MEMORY;
    memset(decl, 0, sizeof(css_declaration_t));

    decl->prop_id = css_property_from_name(name);

    size_t k = 0;
    while (k < sizeof(decl->name) - 1 && name[k] != '\0') {
        decl->name[k] = name[k];
        k++;
    }
    decl->name[k] = '\0';

    // Check !important
    char val_clean[128];
    memset(val_clean, 0, sizeof(val_clean));

    const char* imp = strstr(val_str, "!important");
    if (imp) {
        decl->is_important = true;
        size_t len = imp - val_str;
        if (len >= sizeof(val_clean)) len = sizeof(val_clean) - 1;
        strncpy(val_clean, val_str, len);
        val_clean[len] = '\0';
    } else {
        size_t v_idx = 0;
        while (v_idx < sizeof(val_clean) - 1 && val_str[v_idx] != '\0') {
            val_clean[v_idx] = val_str[v_idx];
            v_idx++;
        }
        val_clean[v_idx] = '\0';
    }

    css_value_parse(val_clean, &decl->value);

    *out_decl = decl;
    return CSS_OK;
}

void css_declaration_destroy(css_declaration_t* decl) {
    if (!decl) return;
    kfree(decl);
}
