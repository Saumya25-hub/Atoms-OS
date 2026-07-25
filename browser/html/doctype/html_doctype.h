#ifndef HTML_DOCTYPE_H
#define HTML_DOCTYPE_H

#include "browser/html/include/html_types.h"

bos_html_status_t html_doctype_create(const char* name, const char* public_id, const char* system_id, bos_node_t** out_doctype);

#endif // HTML_DOCTYPE_H
