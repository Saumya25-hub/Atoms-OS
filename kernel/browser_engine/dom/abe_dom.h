#ifndef ABE_DOM_H
#define ABE_DOM_H

#include "../html/abe_html.h"

#ifdef __cplusplus
extern "C" {
#endif

// DOM Node Engine & Query Selector
typedef struct ABE_DOMNode ABE_DOMNode;

struct ABE_DOMNode {
    uint32_t       node_id;
    ABE_TagType    tag;
    char           tag_name[32];
    char           id_attr[64];
    char           class_attr[128];
    char           text_content[256];
    ABE_DOMNode*   parent;
    ABE_DOMNode*   first_child;
    ABE_DOMNode*   next_sibling;
};

void         ABE_DOM_Init(void);
ABE_DOMNode* ABE_DOM_CreateNode(ABE_TagType tag, const char* name);
void         ABE_DOM_AppendChild(ABE_DOMNode* parent, ABE_DOMNode* child);
ABE_DOMNode* ABE_DOM_QuerySelector(ABE_DOMNode* root, const char* selector);

#ifdef __cplusplus
}
#endif

#endif // ABE_DOM_H
