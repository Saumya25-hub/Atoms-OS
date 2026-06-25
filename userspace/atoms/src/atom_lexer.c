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
    switch (lexer.start[0]) {
        case 'a':
            if (length == 3 && lexer.start[1] == 'n' && lexer.start[2] == 'd') return TOKEN_AND;
            break;
        case 'e':
            if (length == 4 && lexer.start[1] == 'l' && lexer.start[2] == 's' && lexer.start[3] == 'e') return TOKEN_ELSE;
            break;
        case 'f':
            if (length == 4 && lexer.start[1] == 'u' && lexer.start[2] == 'n' && lexer.start[3] == 'c') return TOKEN_FUNC;
            break;
        case 'i':
            if (length == 2 && lexer.start[1] == 'f') return TOKEN_IF;
            break;
        case 'n':
            if (length == 3 && lexer.start[1] == 'o' && lexer.start[2] == 't') return TOKEN_NOT;
            break;
        case 'o':
            if (length == 2 && lexer.start[1] == 'r') return TOKEN_OR;
            break;
        case 'p':
            if (length == 5 && lexer.start[1] == 'r' && lexer.start[2] == 'i' && lexer.start[3] == 'n' && lexer.start[4] == 't') return TOKEN_PRINT;
            break;
        case 'r':
            if (length == 6 && lexer.start[1] == 'e' && lexer.start[2] == 't' && lexer.start[3] == 'u' && lexer.start[4] == 'r' && lexer.start[5] == 'n') return TOKEN_RETURN;
            break;
        case 'w':
            if (length == 5 && lexer.start[1] == 'h' && lexer.start[2] == 'i' && lexer.start[3] == 'l' && lexer.start[4] == 'e') return TOKEN_WHILE;
            break;
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

static Token string(void) {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') lexer.line++;
        advance();
    }
    if (is_at_end()) return error_token("Unterminated string.");
    advance(); // The closing quote
    return make_token(TOKEN_STRING);
}

static bool lexer_match(char expected) {
    if (is_at_end()) return false;
    if (*lexer.current != expected) return false;
    lexer.current++;
    return true;
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
        case '{': return make_token(TOKEN_LBRACE);
        case '}': return make_token(TOKEN_RBRACE);
        case '[': return make_token(TOKEN_LBRACKET);
        case ']': return make_token(TOKEN_RBRACKET);
        case '.': return make_token(TOKEN_DOT);
        case ':': return make_token(TOKEN_COLON);
        case ',': return make_token(TOKEN_COMMA);
        case '+': return make_token(TOKEN_PLUS);
        case '-': return make_token(TOKEN_MINUS);
        case '=': return make_token(lexer_match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '!': return make_token(lexer_match('=') ? TOKEN_BANG_EQUAL : TOKEN_ERROR);
        case '<': return make_token(lexer_match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>': return make_token(lexer_match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string();
    }

    return error_token("Unexpected character.");
}
