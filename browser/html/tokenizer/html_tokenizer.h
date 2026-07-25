#ifndef HTML_TOKENIZER_H
#define HTML_TOKENIZER_H

#include "browser/html/include/html_types.h"

typedef struct {
    const char* input;
    size_t length;
    size_t position;
    size_t line;
    size_t column;
} html_tokenizer_t;

void html_tokenizer_init(html_tokenizer_t* tok, const char* html_input);
bos_html_status_t html_tokenizer_next(html_tokenizer_t* tok, bos_html_token_t* out_token);
void html_token_clear(bos_html_token_t* token);

#endif // HTML_TOKENIZER_H
