#ifndef ABE_CSS_PARSER_H
#define ABE_CSS_PARSER_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_css_tokenizer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_CSS_DECLARATIONS 32
#define ABE_MAX_CSS_RULES 64
#define ABE_MAX_STYLESHEETS 8
#define ABE_MAX_CUSTOM_PROPERTIES 16

typedef enum {
    CSS_VAL_PX = 1,
    CSS_VAL_PERCENT = 2,
    CSS_VAL_EM = 3,
    CSS_VAL_REM = 4,
    CSS_VAL_VW = 5,
    CSS_VAL_VH = 6,
    CSS_VAL_AUTO = 7,
    CSS_VAL_INHERIT = 8,
    CSS_VAL_INITIAL = 9,
    CSS_VAL_COLOR = 10,
    CSS_VAL_IDENT = 11,
    CSS_VAL_VAR = 12,
    CSS_VAL_NONE = 13
} ABE_CSSValueType;

typedef struct {
    ABE_CSSValueType type;
    float number_val;
    char str_val[128];
    uint32_t color_val;
} ABE_CSSValue;

typedef struct {
    char name[64];
    ABE_CSSValue value;
    bool is_important;
} ABE_CSSDeclaration;

typedef struct {
    bool has_media_query;
    float min_width_px;
    float max_width_px;
    float min_height_px;
    float max_height_px;
} ABE_CSSMediaQuery;

typedef struct {
    char selector_str[256];
    ABE_CSSDeclaration declarations[ABE_MAX_CSS_DECLARATIONS];
    uint32_t declaration_count;
    uint32_t rule_index;
    uint32_t specificity_a; // ID
    uint32_t specificity_b; // Class / Attr / Pseudo
    uint32_t specificity_c; // Tag / Pseudo-element
    ABE_CSSMediaQuery media_query;
} ABE_CSSRule;

typedef enum {
    ORIGIN_USER_AGENT = 1,
    ORIGIN_AUTHOR = 2,
    ORIGIN_INLINE = 3
} ABE_CSSOrigin;

typedef struct {
    char name[64];
    char value[128];
} ABE_CSSCustomProperty;

typedef struct {
    ABE_StylesheetHandle handle;
    ABE_CSSRule rules[ABE_MAX_CSS_RULES];
    uint32_t rule_count;
    ABE_CSSCustomProperty custom_properties[ABE_MAX_CUSTOM_PROPERTIES];
    uint32_t custom_property_count;
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
const char*        ABE_CSSParser_GetCustomProperty(ABE_StylesheetHandle handle, const char* name);

#ifdef __cplusplus
}
#endif

#endif // ABE_CSS_PARSER_H

