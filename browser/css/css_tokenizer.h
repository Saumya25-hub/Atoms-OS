#ifndef CSS_TOKENIZER_H
#define CSS_TOKENIZER_H

#include "browser/css/include/css_types.h"

typedef struct {
    css_token_type_t type;
    char value[256];
    char symbol;
} css_token_t;

typedef struct {
    const char* input;
    size_t length;
    size_t position;
    size_t line;
    size_t column;
} css_tokenizer_t;

void css_tokenizer_init(css_tokenizer_t* tok, const char* css_input);
css_status_t css_tokenizer_next(css_tokenizer_t* tok, css_token_t* out_token);
void css_token_clear(css_token_t* token);

#endif // CSS_TOKENIZER_H
