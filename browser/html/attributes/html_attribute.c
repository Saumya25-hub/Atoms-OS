#include "html_attribute.h"
#include "browser/html/diagnostics/html_diagnostics.h"
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

bos_html_status_t html_attr_set(bos_node_t* elem, const char* name, const char* value) {
    if (!elem || !name) return BOS_HTML_ERR_INVALID_PARAM;
    if (elem->type != BOS_NODE_ELEMENT) return BOS_HTML_ERR_NODE_MISMATCH;

    // Check if attribute already exists
    bos_attr_t* curr = elem->attributes;
    while (curr) {
        if (str_case_eq(curr->name, name)) {
            // Update existing value
            strncpy(curr->value, value ? value : "", sizeof(curr->value) - 1);
            curr->value[sizeof(curr->value) - 1] = '\0';
            return BOS_HTML_OK;
        }
        curr = curr->next;
    }

    // Create new attribute
    bos_attr_t* attr = (bos_attr_t*)kmalloc(sizeof(bos_attr_t));
    if (!attr) return BOS_HTML_ERR_OUT_OF_MEMORY;

    memset(attr, 0, sizeof(bos_attr_t));
    strncpy(attr->name, name, sizeof(attr->name) - 1);
    strncpy(attr->value, value ? value : "", sizeof(attr->value) - 1);

    // Prepend to list
    attr->next = elem->attributes;
    elem->attributes = attr;

    html_diag_on_attr_alloc();
    return BOS_HTML_OK;
}

const char* html_attr_get(const bos_node_t* elem, const char* name) {
    if (!elem || !name || elem->type != BOS_NODE_ELEMENT) return NULL;

    bos_attr_t* curr = elem->attributes;
    while (curr) {
        if (str_case_eq(curr->name, name)) {
            return curr->value;
        }
        curr = curr->next;
    }
    return NULL;
}

bos_html_status_t html_attr_remove(bos_node_t* elem, const char* name) {
    if (!elem || !name || elem->type != BOS_NODE_ELEMENT) return BOS_HTML_ERR_INVALID_PARAM;

    bos_attr_t* curr = elem->attributes;
    bos_attr_t* prev = NULL;

    while (curr) {
        if (str_case_eq(curr->name, name)) {
            if (prev) {
                prev->next = curr->next;
            } else {
                elem->attributes = curr->next;
            }
            kfree(curr);
            html_diag_on_attr_free();
            return BOS_HTML_OK;
        }
        prev = curr;
        curr = curr->next;
    }
    return BOS_HTML_ERR_NOT_FOUND;
}

bool html_attr_has(const bos_node_t* elem, const char* name) {
    return (html_attr_get(elem, name) != NULL);
}

void html_attr_clear_all(bos_node_t* elem) {
    if (!elem) return;
    bos_attr_t* curr = elem->attributes;
    while (curr) {
        bos_attr_t* next = curr->next;
        kfree(curr);
        html_diag_on_attr_free();
        curr = next;
    }
    elem->attributes = NULL;
}

bos_attr_t* html_attr_clone_all(const bos_attr_t* src) {
    if (!src) return NULL;
    bos_attr_t* head = NULL;
    bos_attr_t* tail = NULL;

    const bos_attr_t* curr = src;
    while (curr) {
        bos_attr_t* attr = (bos_attr_t*)kmalloc(sizeof(bos_attr_t));
        if (!attr) break;
        memset(attr, 0, sizeof(bos_attr_t));
        strncpy(attr->name, curr->name, sizeof(attr->name) - 1);
        strncpy(attr->value, curr->value, sizeof(attr->value) - 1);

        if (!head) {
            head = attr;
            tail = attr;
        } else {
            tail->next = attr;
            tail = attr;
        }
        html_diag_on_attr_alloc();
        curr = curr->next;
    }
    return head;
}
