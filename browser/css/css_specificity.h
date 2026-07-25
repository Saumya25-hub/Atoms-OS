#ifndef CSS_SPECIFICITY_H
#define CSS_SPECIFICITY_H

#include "browser/css/include/css_types.h"

void css_specificity_calculate(css_selector_t* selector);
int css_specificity_compare(uint32_t a1, uint32_t b1, uint32_t c1,
                            uint32_t a2, uint32_t b2, uint32_t c2);

#endif // CSS_SPECIFICITY_H
