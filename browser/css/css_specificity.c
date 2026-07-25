#include "css_specificity.h"

void css_specificity_calculate(css_selector_t* selector) {
    if (!selector) return;
    selector->spec_a = 0;
    selector->spec_b = 0;
    selector->spec_c = 0;

    css_selector_item_t* item = selector->items_head;
    while (item) {
        if (item->type == CSS_SEL_ID) {
            selector->spec_a++;
        } else if (item->type == CSS_SEL_CLASS) {
            selector->spec_b++;
        } else if (item->type == CSS_SEL_TAG) {
            selector->spec_c++;
        }
        item = item->next;
    }
}

int css_specificity_compare(uint32_t a1, uint32_t b1, uint32_t c1,
                            uint32_t a2, uint32_t b2, uint32_t c2) {
    if (a1 != a2) return (int)a1 - (int)a2;
    if (b1 != b2) return (int)b1 - (int)b2;
    return (int)c1 - (int)c2;
}
