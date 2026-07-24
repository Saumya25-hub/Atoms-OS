#ifndef ATRIX_HTML_PARSER_H
#define ATRIX_HTML_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX HTML5 Parser Subsystem
// ============================================================

typedef enum {
    DOM_NODE_DOCUMENT = 0,
    DOM_NODE_ELEMENT,
    DOM_NODE_TEXT,
    DOM_NODE_COMMENT
} DOMNodeType;

typedef struct DOMNode {
    DOMNodeType     type;
    char            tag_name[32];
    char            text_content[128];
    uint32_t        child_count;
    struct DOMNode* children[16];
} DOMNode;

void     ATRIX_HTMLParser_Init(void);
DOMNode* ATRIX_HTMLParser_ParseString(const char* html_str);
void     ATRIX_DOM_FreeNode(DOMNode* node);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_HTML_PARSER_H
