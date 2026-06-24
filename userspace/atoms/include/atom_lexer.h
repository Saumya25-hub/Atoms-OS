#ifndef ATOM_LEXER_H
#define ATOM_LEXER_H

#include "../../libbos/include/bos.h"

typedef enum {
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_EQUAL,       // =
    TOKEN_PLUS,        // +
    TOKEN_PRINT,       // print
    TOKEN_LPAREN,      // (
    TOKEN_RPAREN,      // )
    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

void atom_lexer_init(const char* source);
Token atom_lexer_scan_token(void);

#endif // ATOM_LEXER_H
