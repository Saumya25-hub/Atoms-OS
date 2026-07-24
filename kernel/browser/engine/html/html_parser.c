#include "html_parser.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ATRIX_HTMLParser_Init(void) {
    bwe_log("INFO", "ATRIX HTML5 Parser Subsystem Initialized");
}

DOMNode* ATRIX_HTMLParser_ParseString(const char* html_str) {
    DOMNode* root = (DOMNode*)kmalloc(sizeof(DOMNode));
    if (!root) return 0;

    root->type = DOM_NODE_DOCUMENT;
    memcpy(root->tag_name, "DOCUMENT", 9);
    root->text_content[0] = '\0';
    root->child_count = 0;

    DOMNode* body = (DOMNode*)kmalloc(sizeof(DOMNode));
    if (body) {
        body->type = DOM_NODE_ELEMENT;
        memcpy(body->tag_name, "body", 5);
        body->text_content[0] = '\0';
        body->child_count = 0;

        root->children[root->child_count++] = body;
    }

    return root;
}

void ATRIX_DOM_FreeNode(DOMNode* node) {
    if (!node) return;
    for (uint32_t i = 0; i < node->child_count; i++) {
        ATRIX_DOM_FreeNode(node->children[i]);
    }
    kfree(node);
}
