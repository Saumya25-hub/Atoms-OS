#include "../include/comctl32_api.h"

extern HCONTROL comctl32_alloc_handle(void);

HCONTROL CreateStatic(HANDLE hwndParent, const char* text, int x, int y, int w, int h, uint32_t style, uint32_t id) {
    (void)hwndParent; (void)text; (void)x; (void)y; (void)w; (void)h; (void)style; (void)id;
    return comctl32_alloc_handle();
}
