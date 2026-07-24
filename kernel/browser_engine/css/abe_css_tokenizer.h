#ifndef ABE_CSS_TOKENIZER_H
#define ABE_CSS_TOKENIZER_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CSSTOKEN_IDENT = 1,
    CSSTOKEN_STRING = 2,
    CSSTOKEN_NUMBER = 3,
    CSSTOKEN_DIMENSION = 4,
    CSSTOKEN_PERCENTAGE = 5,
    CSSTOKEN_HASH = 6,
    CSSTOKEN_DELIM = 7,
    CSSTOKEN_COLON = 8,
    CSSTOKEN_SEMICOLON = 9,
    CSSTOKEN_LBRACE = 10,
    CSSTOKEN_RBRACE = 11,
    CSSTOKEN_LPAREN = 12,
    CSSTOKEN_RPAREN = 13,
    CSSTOKEN_AT_RULE = 14,
    CSSTOKEN_EOF = 15
} ABE_CSSTokenType;

typedef struct {
    ABE_CSSTokenType type;
    char value[256];
    char unit[16];
    float number_val;
    char delim;
} ABE_CSSToken;

typedef struct {
    const char* input;
    size_t input_len;
    size_t position;
} ABE_CSSTokenizer;

ABE_Error ABE_CSSTokenizer_Init(ABE_CSSTokenizer* tok, const char* input, size_t len);
ABE_Error ABE_CSSTokenizer_NextToken(ABE_CSSTokenizer* tok, ABE_CSSToken* out_token);
uint32_t  ABE_CSSTokenizer_ParseColorHex(const char* hex_str);

#ifdef __cplusplus
}
#endif

#endif // ABE_CSS_TOKENIZER_H
