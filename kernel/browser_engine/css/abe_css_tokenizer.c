#include "abe_css_tokenizer.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool IsWhitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f');
}

static bool IsIdentChar(char c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_');
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

    if (c == ':') {
        out_token->type = CSSTOKEN_COLON;
        tok->position++;
        return ABE_SUCCESS;
    }
    if (c == ';') {
        out_token->type = CSSTOKEN_SEMICOLON;
        tok->position++;
        return ABE_SUCCESS;
    }
    if (c == '{') {
        out_token->type = CSSTOKEN_LBRACE;
        tok->position++;
        return ABE_SUCCESS;
    }
    if (c == '}') {
        out_token->type = CSSTOKEN_RBRACE;
        tok->position++;
        return ABE_SUCCESS;
    }
    if (c == '(') {
        out_token->type = CSSTOKEN_LPAREN;
        tok->position++;
        return ABE_SUCCESS;
    }
    if (c == ')') {
        out_token->type = CSSTOKEN_RPAREN;
        tok->position++;
        return ABE_SUCCESS;
    }

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

    // Number or Dimension
    if ((c >= '0' && c <= '9') || c == '.') {
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
        } else if (tok->position < tok->input_len && IsIdentChar(tok->input[tok->position])) {
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

    // Identifier or String
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == '_') {
        out_token->type = CSSTOKEN_IDENT;
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

    // String Quoted
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

    // Delimiter
    out_token->type = CSSTOKEN_DELIM;
    out_token->delim = c;
    tok->position++;
    return ABE_SUCCESS;
}
