#ifndef HTML_TEXT_H
#define HTML_TEXT_H

#include "browser/html/include/html_types.h"

bos_html_status_t html_text_create(const char* text_content, bos_node_t** out_text);
bos_html_status_t html_text_append(bos_node_t* text_node, const char* data);
bos_html_status_t html_text_split(bos_node_t* text_node, uint32_t offset, bos_node_t** out_new_node);

#endif // HTML_TEXT_H
