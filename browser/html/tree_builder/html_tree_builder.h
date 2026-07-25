#ifndef HTML_TREE_BUILDER_H
#define HTML_TREE_BUILDER_H

#include "browser/html/include/html_types.h"

#define MAX_OPEN_ELEMENTS 64

typedef struct {
    bos_document_t* document;
    bos_node_t* stack[MAX_OPEN_ELEMENTS];
    int stack_top;
} html_tree_builder_t;

void html_tree_builder_init(html_tree_builder_t* tb, bos_document_t* doc);
bos_html_status_t html_tree_builder_process_token(html_tree_builder_t* tb, bos_html_token_t* token);
bos_node_t* html_tree_builder_current_node(const html_tree_builder_t* tb);

#endif // HTML_TREE_BUILDER_H
