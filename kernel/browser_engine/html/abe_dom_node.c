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
    node->first_child = NULL;
    node->last_child = NULL;
    node->next_sibling = NULL;
    node->prev_sibling = NULL;
    node->parent = NULL;
    node->child_count = 0;
    node->attribute_count = 0;
    node->tag_name[0] = '\0';
    node->node_value[0] = '\0';
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

// -------------------------------------------------------------
// DOM Query Operations
// -------------------------------------------------------------
static const char* GetAttrValue(const ABE_DOMNode* node, const char* attr_name) {
    if (!node || !attr_name) return NULL;
    for (uint32_t i = 0; i < node->attribute_count; i++) {
        if (strcmp(node->attributes[i].name, attr_name) == 0) {
            return node->attributes[i].value;
        }
    }
    return NULL;
}

static bool ClassListContains(const char* class_attr, const char* target_class) {
    if (!class_attr || !target_class) return false;
    size_t target_len = strlen(target_class);
    if (target_len == 0) return false;

    const char* p = class_attr;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;

        const char* start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        size_t len = (size_t)(p - start);

        if (len == target_len && strncmp(start, target_class, len) == 0) {
            return true;
        }
    }
    return false;
}

ABE_DOMNode* ABE_DOM_GetElementById(const ABE_DOMNode* root, const char* id) {
    if (!root || !id) return NULL;

    if (root->type == ABE_NODE_ELEMENT) {
        const char* node_id = GetAttrValue(root, "id");
        if (node_id && strcmp(node_id, id) == 0) {
            return (ABE_DOMNode*)root;
        }
    }

    ABE_DOMNode* child = root->first_child;
    while (child) {
        ABE_DOMNode* res = ABE_DOM_GetElementById(child, id);
        if (res) return res;
        child = child->next_sibling;
    }
    return NULL;
}

static void TraverseElementsByTagName(const ABE_DOMNode* node, const char* tag_name, ABE_DOMNode** out_array, uint32_t max_count, uint32_t* count) {
    if (!node || !tag_name || !out_array || !count || *count >= max_count) return;

    if (node->type == ABE_NODE_ELEMENT) {
        if (strcmp(tag_name, "*") == 0 || strcmp(node->tag_name, tag_name) == 0) {
            out_array[(*count)++] = (ABE_DOMNode*)node;
            if (*count >= max_count) return;
        }
    }

    ABE_DOMNode* child = node->first_child;
    while (child) {
        TraverseElementsByTagName(child, tag_name, out_array, max_count, count);
        if (*count >= max_count) return;
        child = child->next_sibling;
    }
}

uint32_t ABE_DOM_GetElementsByTagName(const ABE_DOMNode* root, const char* tag_name, ABE_DOMNode** out_array, uint32_t max_count) {
    if (!root || !tag_name || !out_array || max_count == 0) return 0;
    uint32_t count = 0;
    TraverseElementsByTagName(root, tag_name, out_array, max_count, &count);
    return count;
}

static void TraverseElementsByClassName(const ABE_DOMNode* node, const char* class_name, ABE_DOMNode** out_array, uint32_t max_count, uint32_t* count) {
    if (!node || !class_name || !out_array || !count || *count >= max_count) return;

    if (node->type == ABE_NODE_ELEMENT) {
        const char* class_attr = GetAttrValue(node, "class");
        if (class_attr && ClassListContains(class_attr, class_name)) {
            out_array[(*count)++] = (ABE_DOMNode*)node;
            if (*count >= max_count) return;
        }
    }

    ABE_DOMNode* child = node->first_child;
    while (child) {
        TraverseElementsByClassName(child, class_name, out_array, max_count, count);
        if (*count >= max_count) return;
        child = child->next_sibling;
    }
}

uint32_t ABE_DOM_GetElementsByClassName(const ABE_DOMNode* root, const char* class_name, ABE_DOMNode** out_array, uint32_t max_count) {
    if (!root || !class_name || !out_array || max_count == 0) return 0;
    uint32_t count = 0;
    TraverseElementsByClassName(root, class_name, out_array, max_count, &count);
    return count;
}

// -------------------------------------------------------------
// Text Content & Serialization
// -------------------------------------------------------------
static void AppendTextRecursive(const ABE_DOMNode* node, char* out_buf, size_t max_len, size_t* cur_len) {
    if (!node || !out_buf || !cur_len || *cur_len >= max_len - 1) return;

    if (node->type == ABE_NODE_TEXT) {
        size_t val_len = strlen(node->node_value);
        size_t copy_len = (val_len < (max_len - 1 - *cur_len)) ? val_len : (max_len - 1 - *cur_len);
        memcpy(out_buf + *cur_len, node->node_value, copy_len);
        *cur_len += copy_len;
        out_buf[*cur_len] = '\0';
    }

    ABE_DOMNode* child = node->first_child;
    while (child) {
        AppendTextRecursive(child, out_buf, max_len, cur_len);
        child = child->next_sibling;
    }
}

void ABE_DOM_GetTextContent(const ABE_DOMNode* node, char* out_buf, size_t max_len) {
    if (!node || !out_buf || max_len == 0) return;
    out_buf[0] = '\0';
    size_t cur_len = 0;
    AppendTextRecursive(node, out_buf, max_len, &cur_len);
}

void ABE_DOM_SetTextContent(ABE_DOMNode* node, const char* text) {
    if (!node) return;

    // Destroy all existing children
    ABE_DOMNode* child = node->first_child;
    while (child) {
        ABE_DOMNode* next = child->next_sibling;
        ABE_DOM_DestroyNode(child);
        child = next;
    }
    node->first_child = NULL;
    node->last_child = NULL;
    node->child_count = 0;

    if (text && strlen(text) > 0) {
        ABE_DOMNode* txt_node = NULL;
        ABE_DOM_CreateNode(ABE_NODE_TEXT, text, node->owner_document, &txt_node);
        if (txt_node) {
            ABE_DOM_AppendChild(node, txt_node);
        }
    }
}

static void SerializeNode(const ABE_DOMNode* node, char* out_buf, size_t max_len, size_t* cur_len) {
    if (!node || !out_buf || !cur_len || *cur_len >= max_len - 1) return;

    if (node->type == ABE_NODE_TEXT) {
        size_t len = strlen(node->node_value);
        size_t copy_len = (len < max_len - 1 - *cur_len) ? len : (max_len - 1 - *cur_len);
        memcpy(out_buf + *cur_len, node->node_value, copy_len);
        *cur_len += copy_len;
        out_buf[*cur_len] = '\0';
        return;
    } else if (node->type == ABE_NODE_COMMENT) {
        const char* prefix = "<!--";
        const char* suffix = "-->";
        size_t p_len = strlen(prefix), s_len = strlen(suffix), v_len = strlen(node->node_value);
        if (*cur_len + p_len + v_len + s_len < max_len - 1) {
            strcat(out_buf, prefix);
            strcat(out_buf, node->node_value);
            strcat(out_buf, suffix);
            *cur_len += p_len + v_len + s_len;
        }
        return;
    } else if (node->type == ABE_NODE_DOCUMENT_TYPE) {
        const char* dt = "<!DOCTYPE html>";
        size_t dt_len = strlen(dt);
        if (*cur_len + dt_len < max_len - 1) {
            strcat(out_buf, dt);
            *cur_len += dt_len;
        }
        return;
    }

    // Element opening tag
    if (node->type == ABE_NODE_ELEMENT) {
        if (*cur_len + 1 < max_len - 1) { out_buf[(*cur_len)++] = '<'; out_buf[*cur_len] = '\0'; }
        size_t tag_l = strlen(node->tag_name);
        if (*cur_len + tag_l < max_len - 1) { strcat(out_buf, node->tag_name); *cur_len += tag_l; }

        for (uint32_t i = 0; i < node->attribute_count; i++) {
            if (*cur_len + 1 < max_len - 1) { out_buf[(*cur_len)++] = ' '; out_buf[*cur_len] = '\0'; }
            size_t n_l = strlen(node->attributes[i].name);
            if (*cur_len + n_l < max_len - 1) { strcat(out_buf, node->attributes[i].name); *cur_len += n_l; }
            if (*cur_len + 2 < max_len - 1) { strcat(out_buf, "=\""); *cur_len += 2; }
            size_t v_l = strlen(node->attributes[i].value);
            if (*cur_len + v_l < max_len - 1) { strcat(out_buf, node->attributes[i].value); *cur_len += v_l; }
            if (*cur_len + 1 < max_len - 1) { out_buf[(*cur_len)++] = '"'; out_buf[*cur_len] = '\0'; }
        }

        if (*cur_len + 1 < max_len - 1) { out_buf[(*cur_len)++] = '>'; out_buf[*cur_len] = '\0'; }

        // Children
        ABE_DOMNode* child = node->first_child;
        while (child) {
            SerializeNode(child, out_buf, max_len, cur_len);
            child = child->next_sibling;
        }

        // Closing tag
        if (*cur_len + 2 < max_len - 1) { strcat(out_buf, "</"); *cur_len += 2; }
        if (*cur_len + tag_l < max_len - 1) { strcat(out_buf, node->tag_name); *cur_len += tag_l; }
        if (*cur_len + 1 < max_len - 1) { out_buf[(*cur_len)++] = '>'; out_buf[*cur_len] = '\0'; }
    }
}

void ABE_DOM_GetInnerHTML(const ABE_DOMNode* node, char* out_buf, size_t max_len) {
    if (!node || !out_buf || max_len == 0) return;
    out_buf[0] = '\0';
    size_t cur_len = 0;
    ABE_DOMNode* child = node->first_child;
    while (child) {
        SerializeNode(child, out_buf, max_len, &cur_len);
        child = child->next_sibling;
    }
}

void ABE_DOM_GetOuterHTML(const ABE_DOMNode* node, char* out_buf, size_t max_len) {
    if (!node || !out_buf || max_len == 0) return;
    out_buf[0] = '\0';
    size_t cur_len = 0;
    SerializeNode(node, out_buf, max_len, &cur_len);
}
