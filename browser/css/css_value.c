#include "css_value.h"
#include "kernel/core/lib/include/string.h"

static int hex_char_to_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return 0;
}

static bool str_eq_ci(const char* s1, const char* s2) {
    if (!s1 || !s2) return false;
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return false;
        s1++;
        s2++;
    }
    return (*s1 == '\0' && *s2 == '\0');
}

css_status_t css_color_parse(const char* str, css_color_t* out_color) {
    if (!str || !out_color) return CSS_ERR_INVALID_PARAM;
    out_color->r = 0;
    out_color->g = 0;
    out_color->b = 0;
    out_color->a = 255;

    char buf[64];
    size_t idx = 0;
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    while (*str && *str != ' ' && *str != '\t' && *str != '\n' && *str != '\r' && *str != ';' && *str != '}') {
        if (idx < sizeof(buf) - 1) buf[idx++] = *str;
        str++;
    }
    buf[idx] = '\0';
    const char* clean_str = buf;

    // Hex Color #RGB or #RRGGBB
    if (clean_str[0] == '#') {
        clean_str++;
        size_t len = strlen(clean_str);
        if (len == 3) {
            out_color->r = (hex_char_to_val(clean_str[0]) << 4) | hex_char_to_val(clean_str[0]);
            out_color->g = (hex_char_to_val(clean_str[1]) << 4) | hex_char_to_val(clean_str[1]);
            out_color->b = (hex_char_to_val(clean_str[2]) << 4) | hex_char_to_val(clean_str[2]);
            return CSS_OK;
        } else if (len >= 6) {
            out_color->r = (hex_char_to_val(clean_str[0]) << 4) | hex_char_to_val(clean_str[1]);
            out_color->g = (hex_char_to_val(clean_str[2]) << 4) | hex_char_to_val(clean_str[3]);
            out_color->b = (hex_char_to_val(clean_str[4]) << 4) | hex_char_to_val(clean_str[5]);
            return CSS_OK;
        }
    }

    // rgb(r, g, b) or rgba(r, g, b, a)
    if (strncmp(clean_str, "rgb(", 4) == 0 || strncmp(clean_str, "rgba(", 5) == 0) {
        const char* p = clean_str;
        while (*p && *p != '(') p++;
        if (*p == '(') {
            p++;
            int r = 0, g = 0, b = 0, a = 255;

            // Parse integers manually
            while (*p == ' ') p++;
            while (*p >= '0' && *p <= '9') { r = r * 10 + (*p - '0'); p++; }
            while (*p == ' ' || *p == ',') p++;
            while (*p >= '0' && *p <= '9') { g = g * 10 + (*p - '0'); p++; }
            while (*p == ' ' || *p == ',') p++;
            while (*p >= '0' && *p <= '9') { b = b * 10 + (*p - '0'); p++; }
            while (*p == ' ' || *p == ',') p++;
            if (*p >= '0' && *p <= '9') {
                a = 0;
                while (*p >= '0' && *p <= '9') { a = a * 10 + (*p - '0'); p++; }
            }
            out_color->r = (uint8_t)r;
            out_color->g = (uint8_t)g;
            out_color->b = (uint8_t)b;
            out_color->a = (uint8_t)a;
            return CSS_OK;
        }
    }

    // Named Colors
    if (str_eq_ci(clean_str, "red")) { out_color->r = 255; out_color->g = 0; out_color->b = 0; return CSS_OK; }
    if (str_eq_ci(clean_str, "green")) { out_color->r = 0; out_color->g = 128; out_color->b = 0; return CSS_OK; }
    if (str_eq_ci(clean_str, "blue")) { out_color->r = 0; out_color->g = 0; out_color->b = 255; return CSS_OK; }
    if (str_eq_ci(clean_str, "black")) { out_color->r = 0; out_color->g = 0; out_color->b = 0; return CSS_OK; }
    if (str_eq_ci(clean_str, "white")) { out_color->r = 255; out_color->g = 255; out_color->b = 255; return CSS_OK; }
    if (str_eq_ci(clean_str, "gray") || str_eq_ci(clean_str, "grey")) { out_color->r = 128; out_color->g = 128; out_color->b = 128; return CSS_OK; }
    if (str_eq_ci(clean_str, "transparent")) { out_color->r = 0; out_color->g = 0; out_color->b = 0; out_color->a = 0; return CSS_OK; }

    return CSS_ERR_PARSE_ERROR;
}

css_property_id_t css_property_from_name(const char* name) {
    if (!name) return CSS_PROP_UNKNOWN;
    if (str_eq_ci(name, "color")) return CSS_PROP_COLOR;
    if (str_eq_ci(name, "background")) return CSS_PROP_BACKGROUND;
    if (str_eq_ci(name, "background-color")) return CSS_PROP_BACKGROUND_COLOR;
    if (str_eq_ci(name, "width")) return CSS_PROP_WIDTH;
    if (str_eq_ci(name, "height")) return CSS_PROP_HEIGHT;
    if (str_eq_ci(name, "display")) return CSS_PROP_DISPLAY;
    if (str_eq_ci(name, "visibility")) return CSS_PROP_VISIBILITY;
    if (str_eq_ci(name, "opacity")) return CSS_PROP_OPACITY;
    if (str_eq_ci(name, "margin")) return CSS_PROP_MARGIN;
    if (str_eq_ci(name, "margin-left")) return CSS_PROP_MARGIN_LEFT;
    if (str_eq_ci(name, "margin-right")) return CSS_PROP_MARGIN_RIGHT;
    if (str_eq_ci(name, "margin-top")) return CSS_PROP_MARGIN_TOP;
    if (str_eq_ci(name, "margin-bottom")) return CSS_PROP_MARGIN_BOTTOM;
    if (str_eq_ci(name, "padding")) return CSS_PROP_PADDING;
    if (str_eq_ci(name, "padding-left")) return CSS_PROP_PADDING_LEFT;
    if (str_eq_ci(name, "padding-right")) return CSS_PROP_PADDING_RIGHT;
    if (str_eq_ci(name, "padding-top")) return CSS_PROP_PADDING_TOP;
    if (str_eq_ci(name, "padding-bottom")) return CSS_PROP_PADDING_BOTTOM;
    if (str_eq_ci(name, "font-size")) return CSS_PROP_FONT_SIZE;
    if (str_eq_ci(name, "font-family")) return CSS_PROP_FONT_FAMILY;
    if (str_eq_ci(name, "font-weight")) return CSS_PROP_FONT_WEIGHT;
    if (str_eq_ci(name, "text-align")) return CSS_PROP_TEXT_ALIGN;
    if (str_eq_ci(name, "position")) return CSS_PROP_POSITION;
    if (str_eq_ci(name, "top")) return CSS_PROP_TOP;
    if (str_eq_ci(name, "left")) return CSS_PROP_LEFT;
    if (str_eq_ci(name, "right")) return CSS_PROP_RIGHT;
    if (str_eq_ci(name, "bottom")) return CSS_PROP_BOTTOM;
    if (str_eq_ci(name, "overflow")) return CSS_PROP_OVERFLOW;
    return CSS_PROP_UNKNOWN;
}

const char* css_property_to_name(css_property_id_t prop_id) {
    switch (prop_id) {
        case CSS_PROP_COLOR: return "color";
        case CSS_PROP_BACKGROUND: return "background";
        case CSS_PROP_BACKGROUND_COLOR: return "background-color";
        case CSS_PROP_WIDTH: return "width";
        case CSS_PROP_HEIGHT: return "height";
        case CSS_PROP_DISPLAY: return "display";
        case CSS_PROP_VISIBILITY: return "visibility";
        case CSS_PROP_OPACITY: return "opacity";
        case CSS_PROP_MARGIN: return "margin";
        case CSS_PROP_MARGIN_LEFT: return "margin-left";
        case CSS_PROP_MARGIN_RIGHT: return "margin-right";
        case CSS_PROP_MARGIN_TOP: return "margin-top";
        case CSS_PROP_MARGIN_BOTTOM: return "margin-bottom";
        case CSS_PROP_PADDING: return "padding";
        case CSS_PROP_PADDING_LEFT: return "padding-left";
        case CSS_PROP_PADDING_RIGHT: return "padding-right";
        case CSS_PROP_PADDING_TOP: return "padding-top";
        case CSS_PROP_PADDING_BOTTOM: return "padding-bottom";
        case CSS_PROP_FONT_SIZE: return "font-size";
        case CSS_PROP_FONT_FAMILY: return "font-family";
        case CSS_PROP_FONT_WEIGHT: return "font-weight";
        case CSS_PROP_TEXT_ALIGN: return "text-align";
        case CSS_PROP_POSITION: return "position";
        case CSS_PROP_TOP: return "top";
        case CSS_PROP_LEFT: return "left";
        case CSS_PROP_RIGHT: return "right";
        case CSS_PROP_BOTTOM: return "bottom";
        case CSS_PROP_OVERFLOW: return "overflow";
        default: return "unknown";
    }
}

css_status_t css_value_parse(const char* raw_str, css_value_t* out_val) {
    if (!raw_str || !out_val) return CSS_ERR_INVALID_PARAM;
    memset(out_val, 0, sizeof(css_value_t));

    // Safe string copy without strncpy zero-padding overflow
    size_t k = 0;
    while (k < sizeof(out_val->raw_str) - 1 && raw_str[k] != '\0') {
        out_val->raw_str[k] = raw_str[k];
        k++;
    }
    out_val->raw_str[k] = '\0';

    // Check Color
    if (raw_str[0] == '#' || strncmp(raw_str, "rgb(", 4) == 0 || strncmp(raw_str, "rgba(", 5) == 0 ||
        str_eq_ci(raw_str, "red") || str_eq_ci(raw_str, "blue") || str_eq_ci(raw_str, "green") ||
        str_eq_ci(raw_str, "black") || str_eq_ci(raw_str, "white") || str_eq_ci(raw_str, "transparent")) {
        if (css_color_parse(raw_str, &out_val->color_value) == CSS_OK) {
            out_val->type = CSS_VAL_COLOR;
            return CSS_OK;
        }
    }

    // Check Dimension (px, %)
    size_t len = strlen(raw_str);
    if (len > 2 && raw_str[len - 2] == 'p' && raw_str[len - 1] == 'x') {
        out_val->type = CSS_VAL_PX;
        float val = 0;
        int i = 0;
        while (raw_str[i] >= '0' && raw_str[i] <= '9') {
            val = val * 10 + (raw_str[i] - '0');
            i++;
        }
        out_val->number_value = val;
        return CSS_OK;
    }

    if (len > 1 && raw_str[len - 1] == '%') {
        out_val->type = CSS_VAL_PERCENT;
        float val = 0;
        int i = 0;
        while (raw_str[i] >= '0' && raw_str[i] <= '9') {
            val = val * 10 + (raw_str[i] - '0');
            i++;
        }
        out_val->number_value = val;
        return CSS_OK;
    }

    // Pure Number
    bool is_num = true;
    for (size_t idx = 0; idx < len; idx++) {
        if (idx == 0 && raw_str[idx] == '-') continue;
        if ((raw_str[idx] < '0' || raw_str[idx] > '9') && raw_str[idx] != '.') {
            is_num = false;
            break;
        }
    }
    if (is_num && len > 0) {
        out_val->type = CSS_VAL_INTEGER;
        float val = 0;
        int i = (raw_str[0] == '-') ? 1 : 0;
        while (raw_str[i] >= '0' && raw_str[i] <= '9') {
            val = val * 10 + (raw_str[i] - '0');
            i++;
        }
        if (raw_str[0] == '-') val = -val;
        out_val->number_value = val;
        return CSS_OK;
    }

    // Default Keyword / Identifier
    out_val->type = CSS_VAL_KEYWORD;
    return CSS_OK;
}
