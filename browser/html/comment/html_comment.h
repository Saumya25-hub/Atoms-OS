#ifndef HTML_COMMENT_H
#define HTML_COMMENT_H

#include "browser/html/include/html_types.h"

bos_html_status_t html_comment_create(const char* comment_data, bos_node_t** out_comment);

#endif // HTML_COMMENT_H
