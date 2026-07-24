#ifndef ABE_CSS_H
#define ABE_CSS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// CSS Specificity, Cascade & Selector Subsystem
typedef struct {
    char     selector[64];
    uint32_t specificity;
    uint32_t bg_color;
    uint32_t text_color;
    int32_t  font_size;
    int32_t  margin;
    int32_t  padding;
} ABE_CSSRule;

void     ABE_CSS_Init(void);
uint32_t ABE_CSS_CalculateSpecificity(const char* selector);
bool     ABE_CSS_ParseRule(const char* css_snippet, ABE_CSSRule* out_rule);

#ifdef __cplusplus
}
#endif

#endif // ABE_CSS_H
