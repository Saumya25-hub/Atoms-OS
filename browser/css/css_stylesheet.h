#ifndef CSS_STYLESHEET_H
#define CSS_STYLESHEET_H

#include "browser/css/include/css_types.h"

css_status_t css_stylesheet_create(const char* uri, css_stylesheet_t** out_sheet);
css_status_t css_stylesheet_add_rule(css_stylesheet_t* sheet, css_rule_t* rule);
void css_stylesheet_destroy(css_stylesheet_t* sheet);

#endif // CSS_STYLESHEET_H
