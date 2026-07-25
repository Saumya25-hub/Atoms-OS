#include "css_style.h"
#include "css_selector.h"
#include "css_specificity.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static void apply_declaration(css_computed_style_t* style, const css_declaration_t* decl) {
    if (!style || !decl) return;

    switch (decl->prop_id) {
        case CSS_PROP_COLOR:
            style->color = decl->value.color_value;
            style->has_color = true;
            break;

        case CSS_PROP_BACKGROUND:
        case CSS_PROP_BACKGROUND_COLOR:
            style->background_color = decl->value.color_value;
            style->has_bg_color = true;
            break;

        case CSS_PROP_WIDTH:
            style->width = decl->value.number_value;
            style->has_width = true;
            break;

        case CSS_PROP_HEIGHT:
            style->height = decl->value.number_value;
            style->has_height = true;
            break;

        case CSS_PROP_MARGIN:
            style->margin_top = decl->value.number_value;
            style->margin_right = decl->value.number_value;
            style->margin_bottom = decl->value.number_value;
            style->margin_left = decl->value.number_value;
            break;

        case CSS_PROP_MARGIN_LEFT: style->margin_left = decl->value.number_value; break;
        case CSS_PROP_MARGIN_RIGHT: style->margin_right = decl->value.number_value; break;
        case CSS_PROP_MARGIN_TOP: style->margin_top = decl->value.number_value; break;
        case CSS_PROP_MARGIN_BOTTOM: style->margin_bottom = decl->value.number_value; break;

        case CSS_PROP_PADDING:
            style->padding_top = decl->value.number_value;
            style->padding_right = decl->value.number_value;
            style->padding_bottom = decl->value.number_value;
            style->padding_left = decl->value.number_value;
            break;

        case CSS_PROP_PADDING_LEFT: style->padding_left = decl->value.number_value; break;
        case CSS_PROP_PADDING_RIGHT: style->padding_right = decl->value.number_value; break;
        case CSS_PROP_PADDING_TOP: style->padding_top = decl->value.number_value; break;
        case CSS_PROP_PADDING_BOTTOM: style->padding_bottom = decl->value.number_value; break;

        case CSS_PROP_FONT_SIZE:
            style->font_size = decl->value.number_value;
            break;

        case CSS_PROP_OPACITY:
            style->opacity = decl->value.number_value;
            break;

        case CSS_PROP_DISPLAY:
            strncpy(style->display, decl->value.raw_str, sizeof(style->display) - 1);
            style->display[sizeof(style->display) - 1] = '\0';
            break;

        case CSS_PROP_VISIBILITY:
            strncpy(style->visibility, decl->value.raw_str, sizeof(style->visibility) - 1);
            style->visibility[sizeof(style->visibility) - 1] = '\0';
            break;

        case CSS_PROP_POSITION:
            strncpy(style->position, decl->value.raw_str, sizeof(style->position) - 1);
            style->position[sizeof(style->position) - 1] = '\0';
            break;

        case CSS_PROP_FONT_FAMILY:
            strncpy(style->font_family, decl->value.raw_str, sizeof(style->font_family) - 1);
            style->font_family[sizeof(style->font_family) - 1] = '\0';
            break;

        case CSS_PROP_TEXT_ALIGN:
            strncpy(style->text_align, decl->value.raw_str, sizeof(style->text_align) - 1);
            style->text_align[sizeof(style->text_align) - 1] = '\0';
            break;

        case CSS_PROP_OVERFLOW:
            strncpy(style->overflow, decl->value.raw_str, sizeof(style->overflow) - 1);
            style->overflow[sizeof(style->overflow) - 1] = '\0';
            break;

        default:
            break;
    }
}

typedef struct {
    const css_declaration_t* decl;
    uint32_t spec_a;
    uint32_t spec_b;
    uint32_t spec_c;
    uint32_t order;
} matched_decl_t;

css_status_t css_style_compute_for_node(const bos_node_t* node, const css_stylesheet_t* sheet, css_computed_style_t* out_style) {
    if (!node || !out_style) return CSS_ERR_INVALID_PARAM;
    memset(out_style, 0, sizeof(css_computed_style_t));

    // Default style values
    strcpy(out_style->display, "inline");
    strcpy(out_style->visibility, "visible");
    strcpy(out_style->position, "static");
    strcpy(out_style->font_family, "sans-serif");
    out_style->font_size = 16.0f;
    out_style->opacity = 1.0f;
    out_style->color.a = 255;

    if (!sheet || !sheet->rules_head) return CSS_OK;

    // Collect matched declarations
    #define MAX_MATCHED 64
    matched_decl_t matches[MAX_MATCHED];
    uint32_t match_count = 0;
    uint32_t rule_index = 0;

    const css_rule_t* rule = sheet->rules_head;
    while (rule) {
        rule_index++;
        if (rule->type == CSS_RULE_STYLE && rule->selectors) {
            if (css_selector_match_node(rule->selectors, node)) {
                const css_declaration_t* decl = rule->declarations;
                while (decl) {
                    if (match_count < MAX_MATCHED) {
                        matches[match_count].decl = decl;
                        matches[match_count].spec_a = rule->selectors->spec_a;
                        matches[match_count].spec_b = rule->selectors->spec_b;
                        matches[match_count].spec_c = rule->selectors->spec_c;
                        matches[match_count].order = rule_index;
                        match_count++;
                    }
                    decl = decl->next;
                }
            }
        }
        rule = rule->next;
    }

    // Sort matched declarations by specificity, order, and !important
    // Lowest priority comes first, highest priority comes last so it overrides earlier properties
    for (uint32_t i = 0; i < match_count; i++) {
        for (uint32_t j = i + 1; j < match_count; j++) {
            bool swap = false;
            // 1. !important priority (if i is important and j is not, i has higher priority so swap to move i later)
            if (matches[i].decl->is_important && !matches[j].decl->is_important) {
                swap = true;
            } else if (matches[i].decl->is_important == matches[j].decl->is_important) {
                // 2. Specificity (a, b, c)
                int spec_cmp = css_specificity_compare(matches[i].spec_a, matches[i].spec_b, matches[i].spec_c,
                                                        matches[j].spec_a, matches[j].spec_b, matches[j].spec_c);
                if (spec_cmp > 0) {
                    swap = true;
                } else if (spec_cmp == 0) {
                    // 3. Rule order (later rule wins, so if i came later than j, i has higher priority)
                    if (matches[i].order > matches[j].order) {
                        swap = true;
                    }
                }
            }
            if (swap) {
                matched_decl_t tmp = matches[i];
                matches[i] = matches[j];
                matches[j] = tmp;
            }
        }
    }


    // Apply declarations in sorted order
    for (uint32_t i = 0; i < match_count; i++) {
        apply_declaration(out_style, matches[i].decl);
    }

    return CSS_OK;
}

