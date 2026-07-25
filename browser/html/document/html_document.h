#ifndef HTML_DOCUMENT_H
#define HTML_DOCUMENT_H

#include "browser/html/include/html_types.h"

bos_html_status_t html_document_create(const char* uri, bos_document_t** out_doc);
bos_html_status_t html_document_destroy(bos_document_t* doc);
bos_node_t* html_document_get_element_by_id(const bos_document_t* doc, const char* id);
uint32_t html_document_get_elements_by_tag(const bos_document_t* doc, const char* tag, bos_node_t** out_array, uint32_t max_count);
uint32_t html_document_get_elements_by_class(const bos_document_t* doc, const char* class_name, bos_node_t** out_array, uint32_t max_count);

#endif // HTML_DOCUMENT_H
