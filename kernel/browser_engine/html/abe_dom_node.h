#ifndef ABE_DOM_NODE_H
#define ABE_DOM_NODE_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_DOM_POOL_SLOTS 4096

typedef struct ABE_DOMNode {
    ABE_NodeHandle handle;
    ABE_NodeType type;
    char tag_name[64];
    char node_value[512];
    ABE_DOMAttributeInfo attributes[ABE_MAX_ATTRIBUTES];
    uint32_t attribute_count;

    struct ABE_DOMNode* parent;
    struct ABE_DOMNode* first_child;
    struct ABE_DOMNode* last_child;
    struct ABE_DOMNode* prev_sibling;
    struct ABE_DOMNode* next_sibling;

    ABE_DocumentHandle owner_document;
    uint32_t child_count;
    bool in_use;
} ABE_DOMNode;

typedef struct {
    ABE_DOMNode nodes[ABE_DOM_POOL_SLOTS];
    uint32_t active_count;
} ABE_DOMNodePool;

ABE_Error ABE_DOM_InitPool(void);
ABE_Error ABE_DOM_ShutdownPool(void);

ABE_Error ABE_DOM_CreateNode(ABE_NodeType type, const char* tag_or_val, ABE_DocumentHandle owner_doc, ABE_DOMNode** out_node);
ABE_Error ABE_DOM_DestroyNode(ABE_DOMNode* node);

ABE_Error ABE_DOM_AppendChild(ABE_DOMNode* parent, ABE_DOMNode* child);
ABE_Error ABE_DOM_InsertBefore(ABE_DOMNode* parent, ABE_DOMNode* new_node, ABE_DOMNode* ref_node);
ABE_Error ABE_DOM_RemoveChild(ABE_DOMNode* parent, ABE_DOMNode* child);
ABE_Error ABE_DOM_ReplaceChild(ABE_DOMNode* parent, ABE_DOMNode* new_child, ABE_DOMNode* old_child);
ABE_Error ABE_DOM_CloneNode(const ABE_DOMNode* node, bool deep, ABE_DOMNode** out_cloned);

ABE_DOMNode* ABE_DOM_GetNodeByHandle(ABE_NodeHandle handle);

// DOM Query APIs
ABE_DOMNode* ABE_DOM_GetElementById(const ABE_DOMNode* root, const char* id);
uint32_t ABE_DOM_GetElementsByTagName(const ABE_DOMNode* root, const char* tag_name, ABE_DOMNode** out_array, uint32_t max_count);
uint32_t ABE_DOM_GetElementsByClassName(const ABE_DOMNode* root, const char* class_name, ABE_DOMNode** out_array, uint32_t max_count);

// Text Content & Serialization
void ABE_DOM_GetTextContent(const ABE_DOMNode* node, char* out_buf, size_t max_len);
void ABE_DOM_SetTextContent(ABE_DOMNode* node, const char* text);
void ABE_DOM_GetInnerHTML(const ABE_DOMNode* node, char* out_buf, size_t max_len);
void ABE_DOM_GetOuterHTML(const ABE_DOMNode* node, char* out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ABE_DOM_NODE_H

