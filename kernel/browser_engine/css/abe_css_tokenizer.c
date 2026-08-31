#include "abe_css_tokenizer.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool IsWhitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f');
}

static bool IsIdentStart(char c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-');
}

static bool IsIdentChar(char c) {
    return (IsIdentStart(c) || (c >= '0' && c <= '9'));
}

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

static int StrNCaseCmp(const char* s1, const char* s2, size_t n) {
    if (!s1 || !s2) return 1;
    for (size_t i = 0; i < n; i++) {
        char c1 = ToLowerChar(s1[i]);
        char c2 = ToLowerChar(s2[i]);
        if (c1 != c2) return 1;
        if (c1 == '\0') break;
    }
    return 0;
}

static uint32_t HexCharVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return 0;
}

uint32_t ABE_CSSTokenizer_ParseColorHex(const char* hex_str) {
    if (!hex_str) return 0xFF000000;
    if (hex_str[0] == '#') hex_str++;

    size_t len = strlen(hex_str);
    uint32_t r = 0, g = 0, b = 0, a = 255;

    if (len == 3) { // #RGB -> #RRGGBB
        r = HexCharVal(hex_str[0]) * 17;
        g = HexCharVal(hex_str[1]) * 17;
        b = HexCharVal(hex_str[2]) * 17;
    } else if (len == 4) { // #RGBA
        r = HexCharVal(hex_str[0]) * 17;
        g = HexCharVal(hex_str[1]) * 17;
        b = HexCharVal(hex_str[2]) * 17;
        a = HexCharVal(hex_str[3]) * 17;
    } else if (len == 6) { // #RRGGBB
        r = (HexCharVal(hex_str[0]) << 4) | HexCharVal(hex_str[1]);
        g = (HexCharVal(hex_str[2]) << 4) | HexCharVal(hex_str[3]);
        b = (HexCharVal(hex_str[4]) << 4) | HexCharVal(hex_str[5]);
    } else if (len == 8) { // #RRGGBBAA
        r = (HexCharVal(hex_str[0]) << 4) | HexCharVal(hex_str[1]);
        g = (HexCharVal(hex_str[2]) << 4) | HexCharVal(hex_str[3]);
        b = (HexCharVal(hex_str[4]) << 4) | HexCharVal(hex_str[5]);
        a = (HexCharVal(hex_str[6]) << 4) | HexCharVal(hex_str[7]);
    }

    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t ABE_CSSTokenizer_ParseColor(const char* color_str) {
    if (!color_str || color_str[0] == '\0') return 0xFF000000;

    // Hex Color
    if (color_str[0] == '#') {
        return ABE_CSSTokenizer_ParseColorHex(color_str);
    }

    // rgb(...) or rgba(...)
    if (strncmp(color_str, "rgb(", 4) == 0 || strncmp(color_str, "rgba(", 5) == 0) {
        const char* p = color_str;
        while (*p && *p != '(') p++;
        if (*p == '(') {
            p++;
            while (IsWhitespace(*p)) p++;
            int r = 0, g = 0, b = 0;
            float a_f = 1.0f;

            // Parse R
            while (*p >= '0' && *p <= '9') { r = r * 10 + (*p - '0'); p++; }
            while (IsWhitespace(*p) || *p == ',') p++;

            // Parse G
            while (*p >= '0' && *p <= '9') { g = g * 10 + (*p - '0'); p++; }
            while (IsWhitespace(*p) || *p == ',') p++;

            // Parse B
            while (*p >= '0' && *p <= '9') { b = b * 10 + (*p - '0'); p++; }
            while (IsWhitespace(*p) || *p == ',') p++;

            // Optional A
            if (*p >= '0' && *p <= '9') {
                if (*p == '1') a_f = 1.0f;
                else if (*p == '0') {
                    if (*(p + 1) == '.') {
                        a_f = (float)(*(p + 2) - '0') / 10.0f;
                    } else {
                        a_f = 0.0f;
                    }
                }
            }

            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            uint32_t a = (uint32_t)(a_f * 255.0f);
            if (a > 255) a = 255;

            return (a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }

    // Named Colors
    if (StrCaseCmp(color_str, "red") == 0) return 0xFFFF0000;
    if (StrCaseCmp(color_str, "green") == 0) return 0xFF008000;
    if (StrCaseCmp(color_str, "blue") == 0) return 0xFF0000FF;
    if (StrCaseCmp(color_str, "black") == 0) return 0xFF000000;
    if (StrCaseCmp(color_str, "white") == 0) return 0xFFFFFFFF;
    if (StrCaseCmp(color_str, "yellow") == 0) return 0xFFFFFF00;
    if (StrCaseCmp(color_str, "cyan") == 0) return 0xFF00FFFF;
    if (StrCaseCmp(color_str, "magenta") == 0) return 0xFFFF00FF;
    if (StrCaseCmp(color_str, "gray") == 0 || StrCaseCmp(color_str, "grey") == 0) return 0xFF808080;
    if (StrCaseCmp(color_str, "silver") == 0) return 0xFFC0C0C0;
    if (StrCaseCmp(color_str, "maroon") == 0) return 0xFF800000;
    if (StrCaseCmp(color_str, "purple") == 0) return 0xFF800080;
    if (StrCaseCmp(color_str, "navy") == 0) return 0xFF000080;
    if (StrCaseCmp(color_str, "teal") == 0) return 0xFF008080;
    if (StrCaseCmp(color_str, "olive") == 0) return 0xFF808000;
    if (StrCaseCmp(color_str, "orange") == 0) return 0xFFFFA500;
    if (StrCaseCmp(color_str, "transparent") == 0) return 0x00000000;
    if (StrCaseCmp(color_str, "lightgray") == 0) return 0xFFD3D3D3;
    if (StrCaseCmp(color_str, "darkgray") == 0) return 0xFFA9A9A9;
    if (StrCaseCmp(color_str, "crimson") == 0) return 0xFFDC143C;
    if (StrCaseCmp(color_str, "indigo") == 0) return 0xFF4B0082;
    if (StrCaseCmp(color_str, "violet") == 0) return 0xFFEE82EE;
    if (StrCaseCmp(color_str, "pink") == 0) return 0xFFFFC0CB;
    if (StrCaseCmp(color_str, "brown") == 0) return 0xFFA52A2A;

    return 0xFF000000;
}

ABE_Error ABE_CSSTokenizer_Init(ABE_CSSTokenizer* tok, const char* input, size_t len) {
    if (!tok || !input) return ABE_ERR_INVALID_PARAM;
    memset(tok, 0, sizeof(ABE_CSSTokenizer));
    tok->input = input;
    tok->input_len = len;
    tok->position = 0;
    return ABE_SUCCESS;
}

ABE_Error ABE_CSSTokenizer_NextToken(ABE_CSSTokenizer* tok, ABE_CSSToken* out_token) {
    if (!tok || !tok->input || !out_token) return ABE_ERR_INVALID_PARAM;
    memset(out_token, 0, sizeof(ABE_CSSToken));

    // Skip Whitespace & Comments
    while (tok->position < tok->input_len) {
        char c = tok->input[tok->position];
        if (IsWhitespace(c)) {
            tok->position++;
        } else if (c == '/' && tok->position + 1 < tok->input_len && tok->input[tok->position + 1] == '*') {
            // CSS Comment /* ... */
            tok->position += 2;
            while (tok->position + 1 < tok->input_len) {
                if (tok->input[tok->position] == '*' && tok->input[tok->position + 1] == '/') {
                    tok->position += 2;
                    break;
                }
                tok->position++;
            }
        } else {
            break;
        }
    }

    if (tok->position >= tok->input_len) {
        out_token->type = CSSTOKEN_EOF;
        return ABE_SUCCESS;
    }

    char c = tok->input[tok->position];

    // Single Character Punctuation
    if (c == ':') { out_token->type = CSSTOKEN_COLON; tok->position++; return ABE_SUCCESS; }
    if (c == ';') { out_token->type = CSSTOKEN_SEMICOLON; tok->position++; return ABE_SUCCESS; }
    if (c == '{') { out_token->type = CSSTOKEN_LBRACE; tok->position++; return ABE_SUCCESS; }
    if (c == '}') { out_token->type = CSSTOKEN_RBRACE; tok->position++; return ABE_SUCCESS; }
    if (c == '(') { out_token->type = CSSTOKEN_LPAREN; tok->position++; return ABE_SUCCESS; }
    if (c == ')') { out_token->type = CSSTOKEN_RPAREN; tok->position++; return ABE_SUCCESS; }
    if (c == '[') { out_token->type = CSSTOKEN_LBRACKET; tok->position++; return ABE_SUCCESS; }
    if (c == ']') { out_token->type = CSSTOKEN_RBRACKET; tok->position++; return ABE_SUCCESS; }
    if (c == ',') { out_token->type = CSSTOKEN_COMMA; tok->position++; return ABE_SUCCESS; }

    // At-Rule (@media, @import, etc.)
    if (c == '@') {
        tok->position++;
        out_token->type = CSSTOKEN_AT_RULE;
        size_t idx = 0;
        while (tok->position < tok->input_len && IsIdentChar(tok->input[tok->position])) {
            if (idx < sizeof(out_token->value) - 1) {
                out_token->value[idx++] = tok->input[tok->position];
            }
            tok->position++;
        }
        out_token->value[idx] = '\0';
        return ABE_SUCCESS;
    }

    // !important
    if (c == '!') {
        tok->position++;
        while (tok->position < tok->input_len && IsWhitespace(tok->input[tok->position])) tok->position++;
        if (tok->position + 9 <= tok->input_len && StrNCaseCmp(tok->input + tok->position, "important", 9) == 0) {
            tok->position += 9;
            out_token->type = CSSTOKEN_IMPORTANT;
            strncpy(out_token->value, "important", sizeof(out_token->value) - 1);
            return ABE_SUCCESS;
        }
        out_token->type = CSSTOKEN_DELIM;
        out_token->delim = '!';
        return ABE_SUCCESS;
    }

    // Hash Token (#id, #color)
    if (c == '#') {
        out_token->type = CSSTOKEN_HASH;
        tok->position++;
        size_t idx = 0;
        while (tok->position < tok->input_len && IsIdentChar(tok->input[tok->position])) {
            if (idx < sizeof(out_token->value) - 1) {
                out_token->value[idx++] = tok->input[tok->position];
            }
            tok->position++;
        }
        out_token->value[idx] = '\0';
        return ABE_SUCCESS;
    }

    // Numbers, Percentages, and Dimensions
    if ((c >= '0' && c <= '9') || (c == '.' && tok->position + 1 < tok->input_len && tok->input[tok->position + 1] >= '0' && tok->input[tok->position + 1] <= '9')) {
        float val = 0.0f;
        float frac = 0.1f;
        bool in_frac = false;

        while (tok->position < tok->input_len) {
            char ch = tok->input[tok->position];
            if (ch >= '0' && ch <= '9') {
                if (!in_frac) {
                    val = val * 10.0f + (ch - '0');
                } else {
                    val += (ch - '0') * frac;
                    frac *= 0.1f;
                }
                tok->position++;
            } else if (ch == '.' && !in_frac) {
                in_frac = true;
                tok->position++;
            } else {
                break;
            }
        }

        out_token->number_val = val;
        if (tok->position < tok->input_len && tok->input[tok->position] == '%') {
            out_token->type = CSSTOKEN_PERCENTAGE;
            tok->position++;
        } else if (tok->position < tok->input_len && IsIdentStart(tok->input[tok->position])) {
            out_token->type = CSSTOKEN_DIMENSION;
            size_t uidx = 0;
            while (tok->position < tok->input_len && IsIdentChar(tok->input[tok->position])) {
                if (uidx < sizeof(out_token->unit) - 1) {
                    out_token->unit[uidx++] = tok->input[tok->position];
                }
                tok->position++;
            }
            out_token->unit[uidx] = '\0';
        } else {
            out_token->type = CSSTOKEN_NUMBER;
        }
        return ABE_SUCCESS;
    }

    // Identifiers or Function Calls
    if (IsIdentStart(c)) {
        size_t idx = 0;
        while (tok->position < tok->input_len && IsIdentChar(tok->input[tok->position])) {
            if (idx < sizeof(out_token->value) - 1) {
                out_token->value[idx++] = tok->input[tok->position];
            }
            tok->position++;
        }
        out_token->value[idx] = '\0';

        // Check if followed immediately by '(' -> Function call (e.g. rgb(, rgba(, var(, calc()
        if (tok->position < tok->input_len && tok->input[tok->position] == '(') {
            out_token->type = CSSTOKEN_FUNCTION;
            tok->position++; // consume '('
        } else {
            out_token->type = CSSTOKEN_IDENT;
        }
        return ABE_SUCCESS;
    }

    // Quoted Strings
    if (c == '"' || c == '\'') {
        char quote = c;
        tok->position++;
        out_token->type = CSSTOKEN_STRING;
        size_t idx = 0;
        while (tok->position < tok->input_len && tok->input[tok->position] != quote) {
            if (idx < sizeof(out_token->value) - 1) {
                out_token->value[idx++] = tok->input[tok->position];
            }
            tok->position++;
        }
        if (tok->position < tok->input_len) tok->position++; // Skip closing quote
        out_token->value[idx] = '\0';
        return ABE_SUCCESS;
    }

    // Delimiter fallback
    out_token->type = CSSTOKEN_DELIM;
    out_token->delim = c;
    tok->position++;
    return ABE_SUCCESS;
}

