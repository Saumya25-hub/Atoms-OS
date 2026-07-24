#include "abe_dom.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

static uint32_t g_next_dom_id = 1;
extern void bwe_log(const char* level, const char* msg);

void ABE_DOM_Init(void) {
    bwe_log("INFO", "ABE DOM Node Engine & Query Selector Subsystem Initialized");
}

ABE_DOMNode* ABE_DOM_CreateNode(ABE_TagType tag, const char* name) {
    ABE_DOMNode* node = (ABE_DOMNode*)kmalloc(sizeof(ABE_DOMNode));
    if (!node) return 0;
    memset(node, 0, sizeof(ABE_DOMNode));

    node->node_id = g_next_dom_id++;
    node->tag = tag;
    if (name) strncpy(node->tag_name, name, sizeof(node->tag_name) - 1);
    return node;
}

void ABE_DOM_AppendChild(ABE_DOMNode* parent, ABE_DOMNode* child) {
    if (!parent || !child) return;

    child->parent = parent;
    if (!parent->first_child) {
        parent->first_child = child;
    } else {
        ABE_DOMNode* curr = parent->first_child;
        while (curr->next_sibling) {
            curr = curr->next_sibling;
        }
        curr->next_sibling = child;
    }
}

ABE_DOMNode* ABE_DOM_QuerySelector(ABE_DOMNode* root, const char* selector) {
    if (!root || !selector) return 0;

    if (selector[0] == '#') {
        if (strcmp(root->id_attr, selector + 1) == 0) return root;
    } else if (selector[0] == '.') {
        if (strstr(root->class_attr, selector + 1) != 0) return root;
    } else {
        if (strcmp(root->tag_name, selector) == 0) return root;
    }

    ABE_DOMNode* child = root->first_child;
    while (child) {
        ABE_DOMNode* res = ABE_DOM_QuerySelector(child, selector);
        if (res) return res;
        child = child->next_sibling;
    }
    return 0;
}
