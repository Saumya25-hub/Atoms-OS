#ifndef HTML_ATTRIBUTE_H
#define HTML_ATTRIBUTE_H

#include "browser/html/include/html_types.h"

bos_html_status_t html_attr_set(bos_node_t* elem, const char* name, const char* value);
const char* html_attr_get(const bos_node_t* elem, const char* name);
bos_html_status_t html_attr_remove(bos_node_t* elem, const char* name);
bool html_attr_has(const bos_node_t* elem, const char* name);
void html_attr_clear_all(bos_node_t* elem);
bos_attr_t* html_attr_clone_all(const bos_attr_t* src);

#endif // HTML_ATTRIBUTE_H
