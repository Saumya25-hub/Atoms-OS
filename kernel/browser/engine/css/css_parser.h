#ifndef ATRIX_CSS_PARSER_H
#define ATRIX_CSS_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX CSS Parser & Rule Subsystem
// ============================================================

typedef struct {
    char selector[64];
    char property[32];
    char value[64];
    uint32_t parsed_color_argb;
} CSSRule;

typedef struct {
    CSSRule  rules[32];
    uint32_t rule_count;
} CSSStyleSheet;

void          ATRIX_CSSParser_Init(void);
CSSStyleSheet* ATRIX_CSSParser_ParseString(const char* css_str);
uint32_t      ATRIX_CSS_GetColorValue(const char* val_str);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_CSS_PARSER_H
