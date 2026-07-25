#ifndef HTML_PARSER_H
#define HTML_PARSER_H

#include "browser/html/include/html_types.h"

bos_html_status_t html_parser_parse_string(const char* html_input, bos_document_t** out_doc);

#endif // HTML_PARSER_H
