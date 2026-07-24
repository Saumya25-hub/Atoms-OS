#ifndef ABE_CSS_PARSER_H
#define ABE_CSS_PARSER_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_css_tokenizer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_CSS_DECLARATIONS 32
#define ABE_MAX_CSS_RULES 128
#define ABE_MAX_STYLESHEETS 16

typedef enum {
    CSS_VAL_PX = 1,
    CSS_VAL_PERCENT = 2,
    CSS_VAL_EM = 3,
    CSS_VAL_REM = 4,
    CSS_VAL_AUTO = 5,
    CSS_VAL_INHERIT = 6,
    CSS_VAL_INITIAL = 7,
    CSS_VAL_COLOR = 8,
    CSS_VAL_IDENT = 9
} ABE_CSSValueType;

typedef struct {
    ABE_CSSValueType type;
    float number_val;
    char str_val[64];
    uint32_t color_val;
} ABE_CSSValue;

typedef struct {
    char name[64];
    ABE_CSSValue value;
    bool is_important;
} ABE_CSSDeclaration;

typedef struct {
    char selector_str[256];
    ABE_CSSDeclaration declarations[ABE_MAX_CSS_DECLARATIONS];
    uint32_t declaration_count;
    uint32_t rule_index;
    uint32_t specificity_a; // ID
    uint32_t specificity_b; // Class
    uint32_t specificity_c; // Tag
} ABE_CSSRule;

typedef enum {
    ORIGIN_USER_AGENT = 1,
    ORIGIN_AUTHOR = 2,
    ORIGIN_INLINE = 3
} ABE_CSSOrigin;

typedef struct {
    ABE_StylesheetHandle handle;
    ABE_CSSRule rules[ABE_MAX_CSS_RULES];
    uint32_t rule_count;
    ABE_CSSOrigin origin;
    bool in_use;
} ABE_CSSStylesheet;

typedef struct {
    ABE_CSSStylesheet stylesheets[ABE_MAX_STYLESHEETS];
    uint32_t active_count;
} ABE_CSSStylesheetManager;

ABE_Error ABE_CSSParser_Init(void);
ABE_Error ABE_CSSParser_Shutdown(void);

ABE_Error ABE_CSSParser_ParseStylesheet(const char* css_str, size_t len, ABE_CSSOrigin origin, ABE_StylesheetHandle* out_sheet);
ABE_Error ABE_CSSParser_DestroyStylesheet(ABE_StylesheetHandle handle);

ABE_CSSStylesheet* ABE_CSSParser_GetStylesheet(ABE_StylesheetHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ABE_CSS_PARSER_H
