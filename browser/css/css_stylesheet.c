#include "css_stylesheet.h"
#include "css_rule.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

css_status_t css_stylesheet_create(const char* uri, css_stylesheet_t** out_sheet) {
    if (!out_sheet) return CSS_ERR_INVALID_PARAM;
    css_stylesheet_t* sheet = (css_stylesheet_t*)kmalloc(sizeof(css_stylesheet_t));
    if (!sheet) return CSS_ERR_OUT_OF_MEMORY;
    memset(sheet, 0, sizeof(css_stylesheet_t));

    if (uri) {
        size_t k = 0;
        while (k < sizeof(sheet->uri) - 1 && uri[k] != '\0') {
            sheet->uri[k] = uri[k];
            k++;
        }
        sheet->uri[k] = '\0';
    } else {
        strcpy(sheet->uri, "about:blank");
    }

    *out_sheet = sheet;
    return CSS_OK;
}

css_status_t css_stylesheet_add_rule(css_stylesheet_t* sheet, css_rule_t* rule) {
    if (!sheet || !rule) return CSS_ERR_INVALID_PARAM;

    if (!sheet->rules_head) {
        sheet->rules_head = rule;
    } else {
        css_rule_t* curr = sheet->rules_head;
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = rule;
    }
    sheet->rule_count++;
    return CSS_OK;
}

void css_stylesheet_destroy(css_stylesheet_t* sheet) {
    if (!sheet) return;
    if (sheet->rules_head) {
        css_rule_destroy(sheet->rules_head);
    }
    kfree(sheet);
}
