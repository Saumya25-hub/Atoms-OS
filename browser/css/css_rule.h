#ifndef CSS_RULE_H
#define CSS_RULE_H

#include "browser/css/include/css_types.h"

css_status_t css_rule_create(css_rule_type_t type, css_rule_t** out_rule);
void css_rule_destroy(css_rule_t* rule);

css_status_t css_declaration_create(const char* name, const char* val_str, css_declaration_t** out_decl);
void css_declaration_destroy(css_declaration_t* decl);

#endif // CSS_RULE_H
