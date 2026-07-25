#include "html_mutation.h"
#include "browser/html/node/html_node.h"
#include "browser/html/attributes/html_attribute.h"
#include "kernel/core/lib/include/string.h"

bos_html_status_t html_mutation_append_child(bos_node_t* parent, bos_node_t* child) {
    if (!parent || !child) return BOS_HTML_ERR_INVALID_PARAM;

    // Handle DocumentFragment insertion
    if (child->type == BOS_NODE_DOCUMENT_FRAGMENT) {
        bos_node_t* frag_child = child->first_child;
        while (frag_child) {
            bos_node_t* next = frag_child->next_sibling;
            html_mutation_append_child(parent, frag_child);
            frag_child = next;
        }
        child->first_child = NULL;
        child->last_child = NULL;
        child->child_count = 0;
        return BOS_HTML_OK;
    }

    // Detach from current parent if attached
    if (child->parent) {
        html_mutation_remove_child(child->parent, child);
    }

    child->parent = parent;
    child->owner_document = parent->owner_document;
    child->previous_sibling = parent->last_child;
    child->next_sibling = NULL;

    if (parent->last_child) {
        parent->last_child->next_sibling = child;
    } else {
        parent->first_child = child;
    }
    parent->last_child = child;
    parent->child_count++;

    if (parent->owner_document) {
        parent->owner_document->total_nodes++;
    }

    return BOS_HTML_OK;
}

bos_html_status_t html_mutation_remove_child(bos_node_t* parent, bos_node_t* child) {
    if (!parent || !child || child->parent != parent) return BOS_HTML_ERR_INVALID_PARAM;

    if (child->previous_sibling) {
        child->previous_sibling->next_sibling = child->next_sibling;
    } else {
        parent->first_child = child->next_sibling;
    }

    if (child->next_sibling) {
        child->next_sibling->previous_sibling = child->previous_sibling;
    } else {
        parent->last_child = child->previous_sibling;
    }

    child->parent = NULL;
    child->previous_sibling = NULL;
    child->next_sibling = NULL;
    if (parent->child_count > 0) parent->child_count--;

    if (parent->owner_document && parent->owner_document->total_nodes > 0) {
        parent->owner_document->total_nodes--;
    }

    return BOS_HTML_OK;
}

bos_html_status_t html_mutation_replace_child(bos_node_t* parent, bos_node_t* new_child, bos_node_t* old_child) {
    if (!parent || !new_child || !old_child || old_child->parent != parent) return BOS_HTML_ERR_INVALID_PARAM;

    bos_html_status_t status = html_mutation_insert_before(parent, new_child, old_child);
    if (status != BOS_HTML_OK) return status;

    return html_mutation_remove_child(parent, old_child);
}

bos_html_status_t html_mutation_insert_before(bos_node_t* parent, bos_node_t* new_node, bos_node_t* ref_node) {
    if (!parent || !new_node) return BOS_HTML_ERR_INVALID_PARAM;

    if (!ref_node) {
        return html_mutation_append_child(parent, new_node);
    }
    if (ref_node->parent != parent) return BOS_HTML_ERR_INVALID_PARAM;

    if (new_node->parent) {
        html_mutation_remove_child(new_node->parent, new_node);
    }

    new_node->parent = parent;
    new_node->owner_document = parent->owner_document;
    new_node->next_sibling = ref_node;
    new_node->previous_sibling = ref_node->previous_sibling;

    if (ref_node->previous_sibling) {
        ref_node->previous_sibling->next_sibling = new_node;
    } else {
        parent->first_child = new_node;
    }
    ref_node->previous_sibling = new_node;
    parent->child_count++;

    if (parent->owner_document) {
        parent->owner_document->total_nodes++;
    }

    return BOS_HTML_OK;
}

bos_html_status_t html_mutation_clone_node(const bos_node_t* node, bool deep, bos_node_t** out_clone) {
    if (!node || !out_clone) return BOS_HTML_ERR_INVALID_PARAM;

    bos_node_t* clone = NULL;
    bos_html_status_t status = html_node_create(node->type, node->name, &clone);
    if (status != BOS_HTML_OK) return status;

    strncpy(clone->value, node->value, sizeof(clone->value) - 1);
    clone->self_closing = node->self_closing;

    // Clone attributes
    if (node->attributes) {
        clone->attributes = html_attr_clone_all(node->attributes);
    }

    if (deep) {
        bos_node_t* child = node->first_child;
        while (child) {
            bos_node_t* child_clone = NULL;
            if (html_mutation_clone_node(child, true, &child_clone) == BOS_HTML_OK) {
                html_mutation_append_child(clone, child_clone);
            }
            child = child->next_sibling;
        }
    }

    *out_clone = clone;
    return BOS_HTML_OK;
}

bos_html_status_t html_mutation_normalize(bos_node_t* node) {
    if (!node) return BOS_HTML_OK;

    bos_node_t* child = node->first_child;
    while (child) {
        bos_node_t* next = child->next_sibling;
        if (child->type == BOS_NODE_TEXT) {
            // Remove empty text nodes
            if (child->value[0] == '\0') {
                html_mutation_remove_child(node, child);
                html_node_destroy(child);
            } else if (next && next->type == BOS_NODE_TEXT) {
                // Merge adjacent text nodes
                size_t cur_len = strlen(child->value);
                size_t i = 0;
                while (next->value[i] != '\0' && cur_len + i < sizeof(child->value) - 1) {
                    child->value[cur_len + i] = next->value[i];
                    i++;
                }
                child->value[cur_len + i] = '\0';
                html_mutation_remove_child(node, next);
                html_node_destroy(next);
                continue; // Process same child again
            }
        } else {
            html_mutation_normalize(child);
        }
        child = next;
    }
    return BOS_HTML_OK;
}
