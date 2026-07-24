#ifndef ABE_HTML_PARSER_H
#define ABE_HTML_PARSER_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_html_tokenizer.h"
#include "abe_dom_node.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_STACK_DEPTH 256

typedef enum {
    MODE_INITIAL = 0,
    MODE_BEFORE_HTML,
    MODE_BEFORE_HEAD,
    MODE_IN_HEAD,
    MODE_AFTER_HEAD,
    MODE_IN_BODY,
    MODE_IN_TABLE,
    MODE_IN_ROW,
    MODE_IN_CELL,
    MODE_AFTER_BODY,
    MODE_AFTER_AFTER_BODY
} ABE_ParserInsertionMode;

typedef struct {
    ABE_DOMNode* stack[ABE_MAX_STACK_DEPTH];
    uint32_t stack_depth;
    ABE_ParserInsertionMode insertion_mode;
    ABE_DOMNode* document_node;
    ABE_DOMNode* html_node;
    ABE_DOMNode* head_node;
    ABE_DOMNode* body_node;
    bool is_fragment;
} ABE_HTMLParser;

ABE_Error ABE_HTMLParser_Init(void);
ABE_Error ABE_HTMLParser_Shutdown(void);

ABE_Error ABE_HTMLParser_ParseDocument(const char* html_str, size_t len, ABE_DocumentHandle owner_doc, ABE_DOMNode** out_root);
ABE_Error ABE_HTMLParser_PushOpenElement(ABE_HTMLParser* parser, ABE_DOMNode* node);
ABE_DOMNode* ABE_HTMLParser_PopOpenElement(ABE_HTMLParser* parser);
ABE_DOMNode* ABE_HTMLParser_CurrentNode(const ABE_HTMLParser* parser);

#ifdef __cplusplus
}
#endif

#endif // ABE_HTML_PARSER_H
