#ifndef BOS_CSS_TYPES_H
#define BOS_CSS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "browser/html/include/html_types.h"

// ============================================================================
// BOS OS — Phase 5B: CSS Parser & CSSOM Engine Core Types
// ============================================================================

typedef enum {
    CSS_OK                     = 0,
    CSS_ERR_INVALID_PARAM      = -1,
    CSS_ERR_OUT_OF_MEMORY      = -2,
    CSS_ERR_PARSE_ERROR        = -3,
    CSS_ERR_NOT_FOUND          = -4,
    CSS_ERR_BUFFER_OVERFLOW    = -5
} css_status_t;

typedef enum {
    CSS_TOKEN_IDENT            = 1,
    CSS_TOKEN_HASH             = 2,   // #id or #color
    CSS_TOKEN_STRING           = 3,
    CSS_TOKEN_NUMBER           = 4,   // 10, 3.14
    CSS_TOKEN_DIMENSION        = 5,   // 10px, 50%
    CSS_TOKEN_SYMBOL           = 6,   // {, }, :, ;, ,, (, ), [, ]
    CSS_TOKEN_COMBINATOR       = 7,   // >, +, ~, ' '
    CSS_TOKEN_EOF              = 8
} css_token_type_t;

typedef enum {
    CSS_VAL_KEYWORD            = 1,
    CSS_VAL_INTEGER            = 2,
    CSS_VAL_FLOAT              = 3,
    CSS_VAL_PX                 = 4,
    CSS_VAL_PERCENT            = 5,
    CSS_VAL_COLOR              = 6,
    CSS_VAL_STRING             = 7,
    CSS_VAL_IDENTIFIER         = 8,
    CSS_VAL_URL                = 9
} css_value_type_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} css_color_t;

typedef struct {
    css_value_type_t type;
    char raw_str[128];
    float number_value;
    css_color_t color_value;
} css_value_t;

typedef enum {
    CSS_PROP_UNKNOWN           = 0,
    CSS_PROP_COLOR,
    CSS_PROP_BACKGROUND,
    CSS_PROP_BACKGROUND_COLOR,
    CSS_PROP_WIDTH,
    CSS_PROP_HEIGHT,
    CSS_PROP_DISPLAY,
    CSS_PROP_VISIBILITY,
    CSS_PROP_OPACITY,
    CSS_PROP_MARGIN,
    CSS_PROP_MARGIN_LEFT,
    CSS_PROP_MARGIN_RIGHT,
    CSS_PROP_MARGIN_TOP,
    CSS_PROP_MARGIN_BOTTOM,
    CSS_PROP_PADDING,
    CSS_PROP_PADDING_LEFT,
    CSS_PROP_PADDING_RIGHT,
    CSS_PROP_PADDING_TOP,
    CSS_PROP_PADDING_BOTTOM,
    CSS_PROP_FONT_SIZE,
    CSS_PROP_FONT_FAMILY,
    CSS_PROP_FONT_WEIGHT,
    CSS_PROP_TEXT_ALIGN,
    CSS_PROP_POSITION,
    CSS_PROP_TOP,
    CSS_PROP_LEFT,
    CSS_PROP_RIGHT,
    CSS_PROP_BOTTOM,
    CSS_PROP_OVERFLOW
} css_property_id_t;

typedef struct css_declaration {
    css_property_id_t prop_id;
    char name[64];
    css_value_t value;
    bool is_important;
    struct css_declaration* next;
} css_declaration_t;

typedef enum {
    CSS_SEL_TAG                = 1,
    CSS_SEL_CLASS              = 2,
    CSS_SEL_ID                 = 3,
    CSS_SEL_UNIVERSAL          = 4
} css_selector_type_t;

typedef enum {
    CSS_COMB_NONE              = 0,
    CSS_COMB_DESCENDANT        = 1, // ' '
    CSS_COMB_CHILD             = 2, // '>'
    CSS_COMB_ADJACENT_SIBLING  = 3, // '+'
    CSS_COMB_GENERAL_SIBLING   = 4  // '~'
} css_combinator_t;

typedef struct css_selector_item {
    css_selector_type_t type;
    char value[64];
    css_combinator_t combinator; // Combinator to next selector item
    struct css_selector_item* next;
} css_selector_item_t;

typedef struct css_selector {
    css_selector_item_t* items_head;
    uint32_t spec_a; // ID count
    uint32_t spec_b; // Class count
    uint32_t spec_c; // Tag count
    struct css_selector* next_group; // Next selector in group (e.g. h1, h2)
} css_selector_t;

typedef enum {
    CSS_RULE_STYLE             = 1,
    CSS_RULE_MEDIA             = 2,
    CSS_RULE_FONT_FACE         = 3,
    CSS_RULE_KEYFRAMES         = 4
} css_rule_type_t;

typedef struct css_rule {
    css_rule_type_t type;
    css_selector_t* selectors;
    css_declaration_t* declarations;
    struct css_rule* next;
} css_rule_t;

typedef struct css_stylesheet {
    char uri[256];
    css_rule_t* rules_head;
    uint32_t rule_count;
} css_stylesheet_t;

// Computed Style Structure for DOM Node Resolution
typedef struct {
    css_color_t color;
    css_color_t background_color;
    float width;
    float height;
    float margin_top, margin_right, margin_bottom, margin_left;
    float padding_top, padding_right, padding_bottom, padding_left;
    float font_size;
    float opacity;
    char display[32];      // "block", "inline", "none", "flex"
    char visibility[32];   // "visible", "hidden"
    char position[32];     // "static", "relative", "absolute", "fixed"
    char font_family[64];
    char text_align[32];
    char overflow[32];
    bool has_width;
    bool has_height;
    bool has_color;
    bool has_bg_color;
} css_computed_style_t;

#endif // BOS_CSS_TYPES_H
