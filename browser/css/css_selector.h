#ifndef CSS_SELECTOR_H
#define CSS_SELECTOR_H

#include "browser/css/include/css_types.h"
#include "browser/html/include/html_types.h"

css_status_t css_selector_create(css_selector_t** out_selector);
void css_selector_destroy(css_selector_t* selector);
css_status_t css_selector_parse_string(const char* raw_str, css_selector_t** out_selector);
bool css_selector_match_node(const css_selector_t* selector, const bos_node_t* node);

#endif // CSS_SELECTOR_H
