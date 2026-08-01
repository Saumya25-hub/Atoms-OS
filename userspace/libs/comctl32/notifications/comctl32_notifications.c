#include "../include/comctl32_api.h"

BOOL comctl32_notify_parent(HANDLE hwndParent, LPNMHDR pnmh) {
    (void)hwndParent; (void)pnmh;
    return true;
}
