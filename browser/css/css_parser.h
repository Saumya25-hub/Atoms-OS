#ifndef CSS_PARSER_H
#define CSS_PARSER_H

#include "browser/css/include/css_types.h"

css_status_t css_parser_parse_string(const char* css_input, css_stylesheet_t** out_sheet);

#endif // CSS_PARSER_H
