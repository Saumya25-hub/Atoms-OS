#include "../include/atom_lexer.h"
#include "../include/atom_types.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Lexer;

static Lexer lexer;

void atom_lexer_init(const char* source) {
    lexer.start = source;
    lexer.current = source;
    lexer.line = 1;
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_at_end(void) {
    return *lexer.current == '\0';
}

static char advance(void) {
    lexer.current++;
    return lexer.current[-1];
}

static char peek(void) {
    return *lexer.current;
}

static Token make_token(TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer.start;
    token.length = (int)(lexer.current - lexer.start);
    token.line = lexer.line;
    return token;
}

static Token error_token(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = 0;
    while (message[token.length] != '\0') token.length++;
    token.line = lexer.line;
    return token;
}

static void skip_whitespace(void) {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                lexer.line++;
                advance();
                break;
            default:
                return;
        }
    }
}

static TokenType identifier_type(void) {
    int length = (int)(lexer.current - lexer.start);
    if (length == 5) {
        const char* p = "print";
        bool match = true;
        for (int i = 0; i < 5; i++) {
            if (lexer.start[i] != p[i]) match = false;
        }
        if (match) return TOKEN_PRINT;
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier(void) {
    while (is_alpha(peek()) || is_digit(peek())) advance();
    return make_token(identifier_type());
}

static Token number(void) {
    while (is_digit(peek())) advance();
    return make_token(TOKEN_NUMBER);
}

Token atom_lexer_scan_token(void) {
    skip_whitespace();
    lexer.start = lexer.current;

    if (is_at_end()) return make_token(TOKEN_EOF);

    char c = advance();

    if (is_alpha(c)) return identifier();
    if (is_digit(c)) return number();

    switch (c) {
        case '(': return make_token(TOKEN_LPAREN);
        case ')': return make_token(TOKEN_RPAREN);
        case '+': return make_token(TOKEN_PLUS);
        case '=': return make_token(TOKEN_EQUAL);
    }

    return error_token("Unexpected character.");
}
