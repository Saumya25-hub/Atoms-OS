#ifndef HTML_NODE_H
#define HTML_NODE_H

#include "browser/html/include/html_types.h"

bos_html_status_t html_node_create(bos_node_type_t type, const char* name, bos_node_t** out_node);
bos_html_status_t html_node_destroy(bos_node_t* node);
void html_node_add_ref(bos_node_t* node);
void html_node_release_ref(bos_node_t* node);
bool html_node_is_element(const bos_node_t* node);
bool html_node_is_text(const bos_node_t* node);
bool html_node_is_comment(const bos_node_t* node);

#endif // HTML_NODE_H
