#include "abe_css_inherit.h"
#include "kernel/core/lib/include/string.h"

static char ToLowerChar(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

static int StrCaseCmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return 1;
    while (*s1 && *s2) {
        char c1 = ToLowerChar(*s1);
        char c2 = ToLowerChar(*s2);
        if (c1 != c2) return 1;
        s1++; s2++;
    }
    return (*s1 == '\0' && *s2 == '\0') ? 0 : 1;
}

bool ABE_CSSInherit_IsInheritedProperty(const char* property_name) {
    if (!property_name) return false;
    if (StrCaseCmp(property_name, "color") == 0) return true;
    if (StrCaseCmp(property_name, "font-size") == 0) return true;
    if (StrCaseCmp(property_name, "font-weight") == 0) return true;
    if (StrCaseCmp(property_name, "font-family") == 0) return true;
    if (StrCaseCmp(property_name, "line-height") == 0) return true;
    if (StrCaseCmp(property_name, "text-align") == 0) return true;
    if (StrCaseCmp(property_name, "visibility") == 0) return true;
    return false;
}

void ABE_CSSInherit_PropagateInheritedStyles(const ABE_ComputedStyle* parent_style, ABE_ComputedStyle* child_style) {
    if (!parent_style || !child_style) return;

    child_style->color = parent_style->color;
    child_style->font_size_px = parent_style->font_size_px;
    child_style->font_weight = parent_style->font_weight;
    child_style->line_height_px = parent_style->line_height_px;
    child_style->text_align = parent_style->text_align;
    child_style->visibility = parent_style->visibility;
    strncpy(child_style->font_family, parent_style->font_family, sizeof(child_style->font_family) - 1);
}
