#include "../include/comctl32_api.h"

extern HCONTROL comctl32_alloc_handle(void);

HCONTROL CreateIPAddressControl(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style) {
    (void)hwndParent; (void)x; (void)y; (void)w; (void)h; (void)style;
    return comctl32_alloc_handle();
}
