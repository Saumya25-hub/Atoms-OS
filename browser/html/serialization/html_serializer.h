#ifndef HTML_SERIALIZER_H
#define HTML_SERIALIZER_H

#include "browser/html/include/html_types.h"

bos_html_status_t html_serialize_node(const bos_node_t* node, bool pretty, int depth, char* out_buf, uint32_t max_len, uint32_t* out_len);

#endif // HTML_SERIALIZER_H
