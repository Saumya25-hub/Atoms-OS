#include "abe_dom_node.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static ABE_DOMNodePool g_dom_pool;
static uint32_t g_next_node_id = 1000;
static bool g_dom_pool_initialized = false;

ABE_Error ABE_DOM_InitPool(void) {
    memset(&g_dom_pool, 0, sizeof(ABE_DOMNodePool));
    g_dom_pool_initialized = true;
    ABE_Log(ABE_LOG_INFO, "DOM", "ABE Production DOM Node Memory Pool initialized (1024 slab slots)");
    return ABE_SUCCESS;
}

ABE_Error ABE_DOM_ShutdownPool(void) {
    if (!g_dom_pool_initialized) return ABE_ERR_NOT_INITIALIZED;
    g_dom_pool_initialized = false;
    ABE_Log(ABE_LOG_INFO, "DOM", "ABE DOM Node Memory Pool shut down cleanly");
    return ABE_SUCCESS;
}

ABE_DOMNode* ABE_DOM_GetNodeByHandle(ABE_NodeHandle handle) {
    if (!g_dom_pool_initialized || handle == ABE_INVALID_HANDLE) return NULL;
    uint32_t slot = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_DOM_POOL_SLOTS) return NULL;
    if (g_dom_pool.nodes[slot].handle == handle && g_dom_pool.nodes[slot].in_use) {
        return &g_dom_pool.nodes[slot];
    }
    return NULL;
}

ABE_Error ABE_DOM_CreateNode(ABE_NodeType type, const char* tag_or_val, ABE_DocumentHandle owner_doc, ABE_DOMNode** out_node) {
    if (!g_dom_pool_initialized || !out_node) return ABE_ERR_INVALID_PARAM;
    if (g_dom_pool.active_count >= ABE_DOM_POOL_SLOTS) return ABE_ERR_RESOURCE_EXHAUSTED;

    uint32_t slot = ABE_INVALID_HANDLE;
    for (uint32_t i = 0; i < ABE_DOM_POOL_SLOTS; i++) {
        if (!g_dom_pool.nodes[i].in_use) {
            slot = i;
            break;
        }
    }

    if (slot == ABE_INVALID_HANDLE) return ABE_ERR_RESOURCE_EXHAUSTED;

    ABE_DOMNode* node = &g_dom_pool.nodes[slot];
    memset(node, 0, sizeof(ABE_DOMNode));

    node->handle = (g_next_node_id++) | (slot << 16);
    node->type = type;
    node->owner_document = owner_doc;
    node->in_use = true;

    if (tag_or_val) {
        if (type == ABE_NODE_ELEMENT) {
            strncpy(node->tag_name, tag_or_val, sizeof(node->tag_name) - 1);
        } else if (type == ABE_NODE_TEXT || type == ABE_NODE_COMMENT || type == ABE_NODE_DOCUMENT_TYPE) {
            strncpy(node->node_value, tag_or_val, sizeof(node->node_value) - 1);
        }
    }

    g_dom_pool.active_count++;
    ABE_Diag_RecordDOMNodeAllocated();
    *out_node = node;
    return ABE_SUCCESS;
}

ABE_Error ABE_DOM_DestroyNode(ABE_DOMNode* node) {
    if (!g_dom_pool_initialized || !node || !node->in_use) return ABE_ERR_INVALID_PARAM;

    // Recursively destroy children first
    ABE_DOMNode* child = node->first_child;
    while (child) {
        ABE_DOMNode* next = child->next_sibling;
        ABE_DOM_DestroyNode(child);
        child = next;
    }

    node->in_use = false;
    node->handle = ABE_INVALID_HANDLE;
    if (g_dom_pool.active_count > 0) g_dom_pool.active_count--;

    ABE_Diag_RecordDOMNodeFreed();
    return ABE_SUCCESS;
}

ABE_Error ABE_DOM_AppendChild(ABE_DOMNode* parent, ABE_DOMNode* child) {
    if (!parent || !child) return ABE_ERR_INVALID_PARAM;
    if (child->parent) {
        ABE_DOM_RemoveChild(child->parent, child);
    }

    child->parent = parent;
    child->prev_sibling = parent->last_child;
    child->next_sibling = NULL;

    if (parent->last_child) {
        parent->last_child->next_sibling = child;
    } else {
        parent->first_child = child;
    }
    parent->last_child = child;
    parent->child_count++;

    return ABE_SUCCESS;
}

ABE_Error ABE_DOM_InsertBefore(ABE_DOMNode* parent, ABE_DOMNode* new_node, ABE_DOMNode* ref_node) {
    if (!parent || !new_node) return ABE_ERR_INVALID_PARAM;
    if (!ref_node) {
        return ABE_DOM_AppendChild(parent, new_node);
    }

    if (ref_node->parent != parent) return ABE_ERR_DOM_HIERARCHY_ERROR;

    if (new_node->parent) {
        ABE_DOM_RemoveChild(new_node->parent, new_node);
    }

    new_node->parent = parent;
    new_node->next_sibling = ref_node;
    new_node->prev_sibling = ref_node->prev_sibling;

    if (ref_node->prev_sibling) {
        ref_node->prev_sibling->next_sibling = new_node;
    } else {
        parent->first_child = new_node;
    }
    ref_node->prev_sibling = new_node;
    parent->child_count++;

    return ABE_SUCCESS;
}

ABE_Error ABE_DOM_RemoveChild(ABE_DOMNode* parent, ABE_DOMNode* child) {
    if (!parent || !child || child->parent != parent) return ABE_ERR_INVALID_PARAM;

    if (child->prev_sibling) {
        child->prev_sibling->next_sibling = child->next_sibling;
    } else {
        parent->first_child = child->next_sibling;
    }

    if (child->next_sibling) {
        child->next_sibling->prev_sibling = child->prev_sibling;
    } else {
        parent->last_child = child->prev_sibling;
    }

    child->parent = NULL;
    child->prev_sibling = NULL;
    child->next_sibling = NULL;
    if (parent->child_count > 0) parent->child_count--;

    return ABE_SUCCESS;
}

ABE_Error ABE_DOM_ReplaceChild(ABE_DOMNode* parent, ABE_DOMNode* new_child, ABE_DOMNode* old_child) {
    if (!parent || !new_child || !old_child) return ABE_ERR_INVALID_PARAM;
    ABE_Error err = ABE_DOM_InsertBefore(parent, new_child, old_child);
    if (err != ABE_SUCCESS) return err;
    return ABE_DOM_RemoveChild(parent, old_child);
}

ABE_Error ABE_DOM_CloneNode(const ABE_DOMNode* node, bool deep, ABE_DOMNode** out_cloned) {
    if (!node || !out_cloned) return ABE_ERR_INVALID_PARAM;

    ABE_DOMNode* cloned = NULL;
    const char* val = (node->type == ABE_NODE_ELEMENT) ? node->tag_name : node->node_value;
    ABE_Error err = ABE_DOM_CreateNode(node->type, val, node->owner_document, &cloned);
    if (err != ABE_SUCCESS) return err;

    cloned->attribute_count = node->attribute_count;
    memcpy(cloned->attributes, node->attributes, sizeof(node->attributes));

    if (deep) {
        ABE_DOMNode* child = node->first_child;
        while (child) {
            ABE_DOMNode* cloned_child = NULL;
            ABE_DOM_CloneNode(child, true, &cloned_child);
            if (cloned_child) {
                ABE_DOM_AppendChild(cloned, cloned_child);
            }
            child = child->next_sibling;
        }
    }

    *out_cloned = cloned;
    return ABE_SUCCESS;
}
