#ifndef HTML_DEBUG_H
#define HTML_DEBUG_H

#include "browser/html/include/html_types.h"

void html_debug_log(const char* level, const char* message);
void html_debug_dump_node(const bos_node_t* node, int depth);

#endif // HTML_DEBUG_H
