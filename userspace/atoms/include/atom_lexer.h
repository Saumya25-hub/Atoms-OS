#ifndef ATOM_LEXER_H
#define ATOM_LEXER_H

#include "../../libbos/include/bos.h"

typedef enum {
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_EQUAL,         // =
    TOKEN_EQUAL_EQUAL,   // ==
    TOKEN_BANG_EQUAL,    // !=
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_LESS,          // <
    TOKEN_LESS_EQUAL,    // <=
    TOKEN_PLUS,          // +
    TOKEN_MINUS,         // -
    TOKEN_PRINT,         // print
    TOKEN_AND,           // and
    TOKEN_OR,            // or
    TOKEN_NOT,           // not
    TOKEN_IF,            // if
    TOKEN_ELSE,          // else
    TOKEN_WHILE,         // while
    TOKEN_FUNC,          // func
    TOKEN_RETURN,        // return
    TOKEN_LPAREN,        // (
    TOKEN_RPAREN,        // )
    TOKEN_LBRACE,        // {
    TOKEN_RBRACE,        // }
    TOKEN_COMMA,         // ,
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
