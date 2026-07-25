#ifndef CSS_VALUE_H
#define CSS_VALUE_H

#include "browser/css/include/css_types.h"

css_status_t css_value_parse(const char* raw_str, css_value_t* out_val);
css_status_t css_color_parse(const char* color_str, css_color_t* out_color);
css_property_id_t css_property_from_name(const char* name);
const char* css_property_to_name(css_property_id_t prop_id);

#endif // CSS_VALUE_H
