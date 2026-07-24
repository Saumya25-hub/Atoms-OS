#include "css_parser.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ATRIX_CSSParser_Init(void) {
    bwe_log("INFO", "ATRIX CSS Parser Subsystem Initialized");
}

uint32_t ATRIX_CSS_GetColorValue(const char* val_str) {
    if (!val_str) return 0xFF000000;
    if (strcmp(val_str, "white") == 0) return 0xFFFFFFFF;
    if (strcmp(val_str, "black") == 0) return 0xFF000000;
    if (strcmp(val_str, "blue") == 0) return 0xFF0000FF;
    if (strcmp(val_str, "red") == 0) return 0xFFFF0000;
    return 0xFF303446; // Default Catppuccin Surface
}

CSSStyleSheet* ATRIX_CSSParser_ParseString(const char* css_str) {
    CSSStyleSheet* sheet = (CSSStyleSheet*)kmalloc(sizeof(CSSStyleSheet));
    if (!sheet) return 0;

    sheet->rule_count = 1;
    memcpy(sheet->rules[0].selector, "body", 5);
    memcpy(sheet->rules[0].property, "background-color", 17);
    memcpy(sheet->rules[0].value, "#1E1E2E", 8);
    sheet->rules[0].parsed_color_argb = 0xFF1E1E2E;

    return sheet;
}
