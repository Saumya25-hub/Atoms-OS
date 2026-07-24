#ifndef ABE_JS_LEXER_H
#define ABE_JS_LEXER_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_TEMPLATE_LITERAL,
    TOKEN_OPERATOR,
    TOKEN_PUNCTUATION,
    TOKEN_ARROW,           // =>
    TOKEN_OPTIONAL_CHAIN,  // ?.
    TOKEN_NULLISH_COALESCE // ??
} ABE_JSTokenType;

typedef struct {
    ABE_JSTokenType type;
    char text[128];
    double number_val;
    uint32_t line;
    uint32_t column;
} ABE_JSToken;

typedef struct {
    const char* source;
    size_t length;
    size_t cursor;
    uint32_t line;
    uint32_t column;
} ABE_JSLexer;

void ABE_JSLexer_Init(ABE_JSLexer* lexer, const char* source, size_t length);
ABE_Error ABE_JSLexer_NextToken(ABE_JSLexer* lexer, ABE_JSToken* out_token);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_LEXER_H
