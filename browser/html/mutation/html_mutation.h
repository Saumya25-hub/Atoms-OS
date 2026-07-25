#ifndef HTML_MUTATION_H
#define HTML_MUTATION_H

#include "browser/html/include/html_types.h"

bos_html_status_t html_mutation_append_child(bos_node_t* parent, bos_node_t* child);
bos_html_status_t html_mutation_remove_child(bos_node_t* parent, bos_node_t* child);
bos_html_status_t html_mutation_replace_child(bos_node_t* parent, bos_node_t* new_child, bos_node_t* old_child);
bos_html_status_t html_mutation_insert_before(bos_node_t* parent, bos_node_t* new_node, bos_node_t* ref_node);
bos_html_status_t html_mutation_clone_node(const bos_node_t* node, bool deep, bos_node_t** out_clone);
bos_html_status_t html_mutation_normalize(bos_node_t* node);

#endif // HTML_MUTATION_H
