#include "abe_js_lexer.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool IsAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

static bool IsDigit(char c) {
    return (c >= '0' && c <= '9');
}

static bool IsAlphaNum(char c) {
    return IsAlpha(c) || IsDigit(c);
}

static bool IsKeyword(const char* str) {
    const char* keywords[] = {
        "var", "let", "const", "function", "return", "if", "else",
        "for", "while", "class", "async", "await", "import", "export",
        "new", "this", "try", "catch", "throw", "typeof", "instanceof", NULL
    };
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(str, keywords[i]) == 0) return true;
    }
    return false;
}

void ABE_JSLexer_Init(ABE_JSLexer* lexer, const char* source, size_t length) {
    if (!lexer) return;
    lexer->source = source;
    lexer->length = source ? length : 0;
    lexer->cursor = 0;
    lexer->line = 1;
    lexer->column = 1;
}

static void SkipWhitespaceAndComments(ABE_JSLexer* lexer) {
    while (lexer->cursor < lexer->length) {
        char c = lexer->source[lexer->cursor];
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer->cursor++;
            lexer->column++;
        } else if (c == '\n') {
            lexer->cursor++;
            lexer->line++;
            lexer->column = 1;
        } else if (c == '/' && lexer->cursor + 1 < lexer->length && lexer->source[lexer->cursor + 1] == '/') {
            // Line comment
            lexer->cursor += 2;
            while (lexer->cursor < lexer->length && lexer->source[lexer->cursor] != '\n') {
                lexer->cursor++;
            }
        } else if (c == '/' && lexer->cursor + 1 < lexer->length && lexer->source[lexer->cursor + 1] == '*') {
            // Block comment
            lexer->cursor += 2;
            while (lexer->cursor + 1 < lexer->length && !(lexer->source[lexer->cursor] == '*' && lexer->source[lexer->cursor + 1] == '/')) {
                if (lexer->source[lexer->cursor] == '\n') { lexer->line++; lexer->column = 1; }
                else { lexer->column++; }
                lexer->cursor++;
            }
            if (lexer->cursor + 1 < lexer->length) lexer->cursor += 2;
        } else {
            break;
        }
    }
}

ABE_Error ABE_JSLexer_NextToken(ABE_JSLexer* lexer, ABE_JSToken* out_token) {
    if (!lexer || !out_token) return ABE_ERR_INVALID_PARAM;
    memset(out_token, 0, sizeof(ABE_JSToken));

    SkipWhitespaceAndComments(lexer);

    if (lexer->cursor >= lexer->length) {
        out_token->type = TOKEN_EOF;
        return ABE_SUCCESS;
    }

    out_token->line = lexer->line;
    out_token->column = lexer->column;
    char c = lexer->source[lexer->cursor];

    // Arrow Function: =>
    if (c == '=' && lexer->cursor + 1 < lexer->length && lexer->source[lexer->cursor + 1] == '>') {
        out_token->type = TOKEN_ARROW;
        strcpy(out_token->text, "=>");
        lexer->cursor += 2;
        return ABE_SUCCESS;
    }

    // Optional Chaining: ?.
    if (c == '?' && lexer->cursor + 1 < lexer->length && lexer->source[lexer->cursor + 1] == '.') {
        out_token->type = TOKEN_OPTIONAL_CHAIN;
        strcpy(out_token->text, "?.");
        lexer->cursor += 2;
        return ABE_SUCCESS;
    }

    // Nullish Coalescing: ??
    if (c == '?' && lexer->cursor + 1 < lexer->length && lexer->source[lexer->cursor + 1] == '?') {
        out_token->type = TOKEN_NULLISH_COALESCE;
        strcpy(out_token->text, "??");
        lexer->cursor += 2;
        return ABE_SUCCESS;
    }

    // Identifiers & Keywords
    if (IsAlpha(c)) {
        size_t start = lexer->cursor;
        while (lexer->cursor < lexer->length && IsAlphaNum(lexer->source[lexer->cursor])) {
            lexer->cursor++;
        }
        size_t len = lexer->cursor - start;
        if (len >= sizeof(out_token->text)) len = sizeof(out_token->text) - 1;
        strncpy(out_token->text, lexer->source + start, len);
        out_token->text[len] = '\0';

        out_token->type = IsKeyword(out_token->text) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
        return ABE_SUCCESS;
    }

    // Numbers
    if (IsDigit(c)) {
        size_t start = lexer->cursor;
        while (lexer->cursor < lexer->length && (IsDigit(lexer->source[lexer->cursor]) || lexer->source[lexer->cursor] == '.')) {
            lexer->cursor++;
        }
        size_t len = lexer->cursor - start;
        if (len >= sizeof(out_token->text)) len = sizeof(out_token->text) - 1;
        strncpy(out_token->text, lexer->source + start, len);
        out_token->text[len] = '\0';

        out_token->type = TOKEN_NUMBER;
        return ABE_SUCCESS;
    }

    // Strings
    if (c == '"' || c == '\'' || c == '`') {
        char quote = c;
        lexer->cursor++;
        size_t start = lexer->cursor;
        while (lexer->cursor < lexer->length && lexer->source[lexer->cursor] != quote) {
            lexer->cursor++;
        }
        size_t len = lexer->cursor - start;
        if (len >= sizeof(out_token->text)) len = sizeof(out_token->text) - 1;
        strncpy(out_token->text, lexer->source + start, len);
        out_token->text[len] = '\0';

        if (lexer->cursor < lexer->length) lexer->cursor++; // Skip closing quote
        out_token->type = (quote == '`') ? TOKEN_TEMPLATE_LITERAL : TOKEN_STRING;
        return ABE_SUCCESS;
    }

    // Punctuation & Operators
    out_token->text[0] = c;
    out_token->text[1] = '\0';
    lexer->cursor++;

    if (c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']' || c == ';' || c == ',' || c == '.') {
        out_token->type = TOKEN_PUNCTUATION;
    } else {
        out_token->type = TOKEN_OPERATOR;
    }

    return ABE_SUCCESS;
}
