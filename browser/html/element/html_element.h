#ifndef HTML_ELEMENT_H
#define HTML_ELEMENT_H

#include "browser/html/include/html_types.h"

bos_html_status_t html_element_create(const char* tag_name, bos_node_t** out_elem);
const char* html_element_get_id(const bos_node_t* elem);
const char* html_element_get_class(const bos_node_t* elem);

#endif // HTML_ELEMENT_H
