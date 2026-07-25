#include "browser/css/include/bos_css.h"
#include "css_parser.h"
#include "css_stylesheet.h"
#include "css_selector.h"
#include "css_style.h"
#include "css_tests.h"
#include "kernel/drivers/display/display.h"

css_status_t bos_css_init(void) {
    display_print("[CSS ENGINE]\n");
    display_print("Tokenizer Ready\n");
    display_print("Parser Ready\n");
    display_print("CSSOM Ready\n");
    display_print("Selector Engine Ready\n");
    display_print("Specificity Engine Ready\n");
    display_print("Cascade Engine Ready\n");
    return CSS_OK;
}

css_status_t bos_css_parse(const char* css_input, css_stylesheet_t** out_sheet) {
    return css_parser_parse_string(css_input, out_sheet);
}

void bos_css_destroy_stylesheet(css_stylesheet_t* sheet) {
    css_stylesheet_destroy(sheet);
}

bool bos_css_match_selector(const css_selector_t* selector, const bos_node_t* node) {
    return css_selector_match_node(selector, node);
}

css_status_t bos_css_compute_style(const bos_node_t* node, const css_stylesheet_t* sheet, css_computed_style_t* out_style) {
    return css_style_compute_for_node(node, sheet, out_style);
}

void bos_css_dump_stylesheet(const css_stylesheet_t* sheet) {
    if (!sheet) return;
    display_print("--- CSS Stylesheet Dump ---\n");
    display_print("URI: "); display_print(sheet->uri); display_print("\n");
    display_print("Rules Count: "); display_print_dec(sheet->rule_count); display_print("\n");

    const css_rule_t* rule = sheet->rules_head;
    uint32_t idx = 1;
    while (rule) {
        display_print("Rule #"); display_print_dec(idx++); display_print("\n");
        if (rule->selectors) {
            display_print("  Specificity: (");
            display_print_dec(rule->selectors->spec_a); display_print(",");
            display_print_dec(rule->selectors->spec_b); display_print(",");
            display_print_dec(rule->selectors->spec_c); display_print(")\n");
        }
        const css_declaration_t* decl = rule->declarations;
        while (decl) {
            display_print("    "); display_print(decl->name);
            display_print(": "); display_print(decl->value.raw_str);
            if (decl->is_important) display_print(" !important");
            display_print("\n");
            decl = decl->next;
        }
        rule = rule->next;
    }
    display_print("---------------------------\n");
}

void bos_css_run_certification_tests(void) {
    css_run_all_certification_tests();
}
