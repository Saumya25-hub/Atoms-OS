#include "../include/comctl32_api.h"

extern HCONTROL comctl32_alloc_handle(void);

HCONTROL CreateUpDownControl(HANDLE hwndParent, uint32_t style, HANDLE hwndBuddy) {
    (void)hwndParent; (void)style; (void)hwndBuddy;
    return comctl32_alloc_handle();
}
