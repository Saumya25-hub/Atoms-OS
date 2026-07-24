#ifndef ABE_CSS_INHERIT_H
#define ABE_CSS_INHERIT_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ABE_CSSInherit_IsInheritedProperty(const char* property_name);
void ABE_CSSInherit_PropagateInheritedStyles(const ABE_ComputedStyle* parent_style, ABE_ComputedStyle* child_style);

#ifdef __cplusplus
}
#endif

#endif // ABE_CSS_INHERIT_H
