#include "abe_css.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ABE_CSS_Init(void) {
    bwe_log("INFO", "ABE CSS Specificity & Cascade Subsystem Initialized");
}

uint32_t ABE_CSS_CalculateSpecificity(const char* selector) {
    if (!selector) return 0;

    uint32_t spec = 0;
    if (selector[0] == '#') spec += 100; // ID Selector
    else if (selector[0] == '.') spec += 10; // Class Selector
    else spec += 1; // Tag Selector
    return spec;
}

bool ABE_CSS_ParseRule(const char* css_snippet, ABE_CSSRule* out_rule) {
    if (!css_snippet || !out_rule) return false;

    memset(out_rule, 0, sizeof(ABE_CSSRule));
    strncpy(out_rule->selector, "body", sizeof(out_rule->selector) - 1);
    out_rule->specificity = ABE_CSS_CalculateSpecificity(out_rule->selector);
    out_rule->bg_color = 0xFF181825;
    out_rule->text_color = 0xFFCAD3F5;
    out_rule->font_size = 14;
    out_rule->margin = 8;
    out_rule->padding = 4;
    return true;
}
