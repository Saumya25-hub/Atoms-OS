#include "html_document.h"
#include "browser/html/node/html_node.h"
#include "browser/html/attributes/html_attribute.h"
#include "kernel/core/lib/include/string.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

static bool str_case_eq(const char* s1, const char* s2) {
    if (!s1 || !s2) return false;
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return false;
        s1++;
        s2++;
    }
    return (*s1 == '\0' && *s2 == '\0');
}

bos_html_status_t html_document_create(const char* uri, bos_document_t** out_doc) {
    if (!out_doc) return BOS_HTML_ERR_INVALID_PARAM;
    *out_doc = NULL;

    bos_document_t* doc = (bos_document_t*)kmalloc(sizeof(bos_document_t));
    if (!doc) return BOS_HTML_ERR_OUT_OF_MEMORY;

    memset(doc, 0, sizeof(bos_document_t));
    strncpy(doc->document_uri, uri ? uri : "about:blank", sizeof(doc->document_uri) - 1);
    strncpy(doc->character_encoding, "UTF-8", sizeof(doc->character_encoding) - 1);
    strncpy(doc->ready_state, "complete", sizeof(doc->ready_state) - 1);

    // Create Root Document Node
    bos_node_t* root = NULL;
    bos_html_status_t status = html_node_create(BOS_NODE_DOCUMENT, "#document", &root);
    if (status != BOS_HTML_OK) {
        kfree(doc);
        return status;
    }

    root->owner_document = doc;
    doc->root_node = root;
    doc->total_nodes = 1;

    *out_doc = doc;
    return BOS_HTML_OK;
}

bos_html_status_t html_document_destroy(bos_document_t* doc) {
    if (!doc) return BOS_HTML_OK;
    if (doc->root_node) {
        html_node_destroy(doc->root_node);
        doc->root_node = NULL;
    }
    kfree(doc);
    return BOS_HTML_OK;
}

static bos_node_t* find_by_id_recursive(bos_node_t* node, const char* id) {
    if (!node) return NULL;
    if (node->type == BOS_NODE_ELEMENT) {
        const char* val = html_attr_get(node, "id");
        if (val && strcmp(val, id) == 0) {
            return node;
        }
    }
    bos_node_t* child = node->first_child;
    while (child) {
        bos_node_t* found = find_by_id_recursive(child, id);
        if (found) return found;
        child = child->next_sibling;
    }
    return NULL;
}

bos_node_t* html_document_get_element_by_id(const bos_document_t* doc, const char* id) {
    if (!doc || !doc->root_node || !id) return NULL;
    return find_by_id_recursive(doc->root_node, id);
}

static void find_by_tag_recursive(bos_node_t* node, const char* tag, bos_node_t** out_array, uint32_t max_count, uint32_t* count) {
    if (!node || *count >= max_count) return;
    if (node->type == BOS_NODE_ELEMENT) {
        if (strcmp(tag, "*") == 0 || str_case_eq(node->name, tag)) {
            out_array[*count] = node;
            (*count)++;
        }
    }
    bos_node_t* child = node->first_child;
    while (child && *count < max_count) {
        find_by_tag_recursive(child, tag, out_array, max_count, count);
        child = child->next_sibling;
    }
}

uint32_t html_document_get_elements_by_tag(const bos_document_t* doc, const char* tag, bos_node_t** out_array, uint32_t max_count) {
    if (!doc || !doc->root_node || !tag || !out_array || max_count == 0) return 0;
    uint32_t count = 0;
    find_by_tag_recursive(doc->root_node, tag, out_array, max_count, &count);
    return count;
}

static void find_by_class_recursive(bos_node_t* node, const char* class_name, bos_node_t** out_array, uint32_t max_count, uint32_t* count) {
    if (!node || *count >= max_count) return;
    if (node->type == BOS_NODE_ELEMENT) {
        const char* val = html_attr_get(node, "class");
        if (val && strstr(val, class_name) != NULL) {
            out_array[*count] = node;
            (*count)++;
        }
    }
    bos_node_t* child = node->first_child;
    while (child && *count < max_count) {
        find_by_class_recursive(child, class_name, out_array, max_count, count);
        child = child->next_sibling;
    }
}

uint32_t html_document_get_elements_by_class(const bos_document_t* doc, const char* class_name, bos_node_t** out_array, uint32_t max_count) {
    if (!doc || !doc->root_node || !class_name || !out_array || max_count == 0) return 0;
    uint32_t count = 0;
    find_by_class_recursive(doc->root_node, class_name, out_array, max_count, &count);
    return count;
}
