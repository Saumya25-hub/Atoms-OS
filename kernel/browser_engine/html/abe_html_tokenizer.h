#ifndef ABE_HTML_TOKENIZER_H
#define ABE_HTML_TOKENIZER_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TOKEN_DOCTYPE = 1,
    TOKEN_START_TAG = 2,
    TOKEN_END_TAG = 3,
    TOKEN_COMMENT = 4,
    TOKEN_CHARACTER = 5,
    TOKEN_EOF = 6
} ABE_TokenType;

typedef enum {
    STATE_DATA = 0,
    STATE_TAG_OPEN,
    STATE_END_TAG_OPEN,
    STATE_TAG_NAME,
    STATE_BEFORE_ATTR_NAME,
    STATE_ATTR_NAME,
    STATE_AFTER_ATTR_NAME,
    STATE_BEFORE_ATTR_VAL,
    STATE_ATTR_VAL_DOUBLE_QUOTED,
    STATE_ATTR_VAL_SINGLE_QUOTED,
    STATE_ATTR_VAL_UNQUOTED,
    STATE_AFTER_ATTR_VAL,
    STATE_SELF_CLOSING_TAG,
    STATE_COMMENT,
    STATE_DOCTYPE,
    STATE_BEFORE_DOCTYPE_NAME,
    STATE_DOCTYPE_NAME,
    STATE_AFTER_DOCTYPE_NAME,
    STATE_RAWTEXT,
    STATE_RAWTEXT_LESS_THAN,
    STATE_RAWTEXT_END_TAG_OPEN,
    STATE_RAWTEXT_END_TAG_NAME
} ABE_TokenizerState;

typedef struct {
    ABE_TokenType type;
    char tag_name[64];
    char value[512];
    ABE_DOMAttributeInfo attributes[ABE_MAX_ATTRIBUTES];
    uint32_t attribute_count;
    bool self_closing;
} ABE_HTMLToken;

typedef struct {
    const char* input;
    size_t input_len;
    size_t position;
    ABE_TokenizerState state;
    char current_attr_name[64];
    char current_attr_val[256];
    char rawtext_tag[64];
    size_t rawtext_end_tag_idx;
} ABE_HTMLTokenizer;

ABE_Error ABE_HTMLTokenizer_Init(ABE_HTMLTokenizer* tok, const char* input, size_t len);
ABE_Error ABE_HTMLTokenizer_NextToken(ABE_HTMLTokenizer* tok, ABE_HTMLToken* out_token);
void      ABE_HTMLTokenizer_SwitchToRawText(ABE_HTMLTokenizer* tok, const char* tag);
void      ABE_HTMLTokenizer_DecodeEntities(const char* in_str, char* out_str, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ABE_HTML_TOKENIZER_H

