#ifndef CSS_STYLE_H
#define CSS_STYLE_H

#include "browser/css/include/css_types.h"
#include "browser/html/include/html_types.h"

css_status_t css_style_compute_for_node(const bos_node_t* node, const css_stylesheet_t* sheet, css_computed_style_t* out_style);

#endif // CSS_STYLE_H
