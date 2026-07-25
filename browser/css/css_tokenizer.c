#include "css_tokenizer.h"
#include "kernel/core/lib/include/string.h"

void css_tokenizer_init(css_tokenizer_t* tok, const char* css_input) {
    if (!tok) return;
    tok->input = css_input ? css_input : "";
    tok->length = strlen(tok->input);
    tok->position = 0;
    tok->line = 1;
    tok->column = 1;
}

void css_token_clear(css_token_t* token) {
    if (!token) return;
    memset(token, 0, sizeof(css_token_t));
}

static char peek_char(css_tokenizer_t* tok) {
    if (tok->position >= tok->length) return '\0';
    return tok->input[tok->position];
}

static char get_char(css_tokenizer_t* tok) {
    if (tok->position >= tok->length) return '\0';
    char c = tok->input[tok->position++];
    if (c == '\n') {
        tok->line++;
        tok->column = 1;
    } else {
        tok->column++;
    }
    return c;
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-';
}

static bool is_digit(char c) {
    return (c >= '0' && c <= '9');
}

static bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void skip_comments_and_whitespace(css_tokenizer_t* tok) {
    while (tok->position < tok->length) {
        char c = peek_char(tok);
        if (is_whitespace(c)) {
            get_char(tok);
            continue;
        }
        // Comment /* ... */
        if (c == '/' && tok->position + 1 < tok->length && tok->input[tok->position + 1] == '*') {
            get_char(tok); // consume '/'
            get_char(tok); // consume '*'
            while (tok->position < tok->length) {
                if (peek_char(tok) == '*' && tok->position + 1 < tok->length && tok->input[tok->position + 1] == '/') {
                    get_char(tok); // consume '*'
                    get_char(tok); // consume '/'
                    break;
                }
                get_char(tok);
            }
            continue;
        }
        break;
    }
}

css_status_t css_tokenizer_next(css_tokenizer_t* tok, css_token_t* out_token) {
    if (!tok || !out_token) return CSS_ERR_INVALID_PARAM;
    css_token_clear(out_token);

    skip_comments_and_whitespace(tok);

    if (tok->position >= tok->length) {
        out_token->type = CSS_TOKEN_EOF;
        return CSS_OK;
    }

    char c = peek_char(tok);

    // Symbols
    if (c == '{' || c == '}' || c == ':' || c == ';' || c == ',' || c == '(' || c == ')' || c == '[' || c == ']') {
        out_token->type = CSS_TOKEN_SYMBOL;
        out_token->symbol = get_char(tok);
        out_token->value[0] = out_token->symbol;
        out_token->value[1] = '\0';
        return CSS_OK;
    }

    // Combinators
    if (c == '>' || c == '+' || c == '~') {
        out_token->type = CSS_TOKEN_COMBINATOR;
        out_token->symbol = get_char(tok);
        out_token->value[0] = out_token->symbol;
        out_token->value[1] = '\0';
        return CSS_OK;
    }

    // Hash / Hex Color / ID selector (#main, #ffffff)
    if (c == '#') {
        get_char(tok); // consume '#'
        out_token->type = CSS_TOKEN_HASH;
        size_t idx = 0;
        while (tok->position < tok->length && is_alnum(peek_char(tok))) {
            if (idx < sizeof(out_token->value) - 1) {
                out_token->value[idx++] = get_char(tok);
            } else {
                get_char(tok);
            }
        }
        out_token->value[idx] = '\0';
        return CSS_OK;
    }

    // Class selector (.class)
    if (c == '.' && tok->position + 1 < tok->length && is_alpha(tok->input[tok->position + 1])) {
        get_char(tok); // consume '.'
        out_token->type = CSS_TOKEN_IDENT;
        out_token->value[0] = '.';
        size_t idx = 1;
        while (tok->position < tok->length && is_alnum(peek_char(tok))) {
            if (idx < sizeof(out_token->value) - 1) {
                out_token->value[idx++] = get_char(tok);
            } else {
                get_char(tok);
            }
        }
        out_token->value[idx] = '\0';
        return CSS_OK;
    }

    // Universal Selector (*)
    if (c == '*') {
        get_char(tok); // consume '*'
        out_token->type = CSS_TOKEN_IDENT;
        out_token->value[0] = '*';
        out_token->value[1] = '\0';
        return CSS_OK;
    }

    // String ("..." or '...')
    if (c == '"' || c == '\'') {
        char quote = get_char(tok);
        out_token->type = CSS_TOKEN_STRING;
        size_t idx = 0;
        while (tok->position < tok->length && peek_char(tok) != quote) {
            if (idx < sizeof(out_token->value) - 1) {
                out_token->value[idx++] = get_char(tok);
            } else {
                get_char(tok);
            }
        }
        out_token->value[idx] = '\0';
        if (peek_char(tok) == quote) get_char(tok);
        return CSS_OK;
    }

    // Number / Dimension (10, 3.14, 10px, 50%)
    if (is_digit(c) || (c == '-' && is_digit(tok->input[tok->position + 1]))) {
        size_t idx = 0;
        if (c == '-') out_token->value[idx++] = get_char(tok);

        while (tok->position < tok->length && (is_digit(peek_char(tok)) || peek_char(tok) == '.')) {
            if (idx < sizeof(out_token->value) - 1) {
                out_token->value[idx++] = get_char(tok);
            } else {
                get_char(tok);
            }
        }

        // Check if followed by unit (px, %, em, etc.)
        if (peek_char(tok) == '%') {
            out_token->type = CSS_TOKEN_DIMENSION;
            if (idx < sizeof(out_token->value) - 1) out_token->value[idx++] = get_char(tok);
        } else if (is_alpha(peek_char(tok))) {
            out_token->type = CSS_TOKEN_DIMENSION;
            while (tok->position < tok->length && is_alpha(peek_char(tok))) {
                if (idx < sizeof(out_token->value) - 1) {
                    out_token->value[idx++] = get_char(tok);
                } else {
                    get_char(tok);
                }
            }
        } else {
            out_token->type = CSS_TOKEN_NUMBER;
        }
        out_token->value[idx] = '\0';
        return CSS_OK;
    }

    // Identifiers (div, body, color, margin-left, etc.)
    if (is_alpha(c)) {
        out_token->type = CSS_TOKEN_IDENT;
        size_t idx = 0;
        while (tok->position < tok->length && is_alnum(peek_char(tok))) {
            if (idx < sizeof(out_token->value) - 1) {
                out_token->value[idx++] = get_char(tok);
            } else {
                get_char(tok);
            }
        }
        out_token->value[idx] = '\0';
        return CSS_OK;
    }

    // Fallback: Consume single unhandled char as symbol
    out_token->type = CSS_TOKEN_SYMBOL;
    out_token->symbol = get_char(tok);
    out_token->value[0] = out_token->symbol;
    out_token->value[1] = '\0';
    return CSS_OK;
}
