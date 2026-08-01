#include "../include/comctl32_api.h"

extern HCONTROL comctl32_alloc_handle(void);

HCONTROL CreateToolTip(HANDLE hwndParent, uint32_t style) {
    (void)hwndParent; (void)style;
    return comctl32_alloc_handle();
}
