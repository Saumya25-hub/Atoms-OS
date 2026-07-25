#include "html_node.h"
#include "browser/html/attributes/html_attribute.h"
#include "browser/html/diagnostics/html_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

static uint32_t g_next_node_id = 1;

static void to_lowercase(char* str) {
    if (!str) return;
    while (*str) {
        if (*str >= 'A' && *str <= 'Z') {
            *str += 32;
        }
        str++;
    }
}

bos_html_status_t html_node_create(bos_node_type_t type, const char* name, bos_node_t** out_node) {
    if (!out_node) return BOS_HTML_ERR_INVALID_PARAM;
    *out_node = NULL;

    bos_node_t* node = (bos_node_t*)kmalloc(sizeof(bos_node_t));
    if (!node) return BOS_HTML_ERR_OUT_OF_MEMORY;

    memset(node, 0, sizeof(bos_node_t));
    node->id = g_next_node_id++;
    node->type = type;
    node->ref_count = 1;

    if (name) {
        strncpy(node->name, name, sizeof(node->name) - 1);
        if (type == BOS_NODE_ELEMENT) {
            to_lowercase(node->name);
        }
    } else {
        if (type == BOS_NODE_TEXT) strncpy(node->name, "#text", sizeof(node->name));
        else if (type == BOS_NODE_COMMENT) strncpy(node->name, "#comment", sizeof(node->name));
        else if (type == BOS_NODE_DOCUMENT) strncpy(node->name, "#document", sizeof(node->name));
        else if (type == BOS_NODE_DOCUMENT_FRAGMENT) strncpy(node->name, "#document-fragment", sizeof(node->name));
    }

    html_diag_on_node_alloc();
    *out_node = node;
    return BOS_HTML_OK;
}

bos_html_status_t html_node_destroy(bos_node_t* node) {
    if (!node) return BOS_HTML_OK;

    // Recursively destroy children
    bos_node_t* child = node->first_child;
    while (child) {
        bos_node_t* next = child->next_sibling;
        child->parent = NULL;
        child->previous_sibling = NULL;
        child->next_sibling = NULL;
        html_node_destroy(child);
        child = next;
    }

    // Clear attributes
    html_attr_clear_all(node);

    html_diag_on_node_free();
    kfree(node);
    return BOS_HTML_OK;
}

void html_node_add_ref(bos_node_t* node) {
    if (node) {
        node->ref_count++;
    }
}

void html_node_release_ref(bos_node_t* node) {
    if (!node) return;
    if (node->ref_count > 0) {
        node->ref_count--;
    }
    if (node->ref_count == 0) {
        html_node_destroy(node);
    }
}

bool html_node_is_element(const bos_node_t* node) {
    return (node && node->type == BOS_NODE_ELEMENT);
}

bool html_node_is_text(const bos_node_t* node) {
    return (node && node->type == BOS_NODE_TEXT);
}

bool html_node_is_comment(const bos_node_t* node) {
    return (node && node->type == BOS_NODE_COMMENT);
}
