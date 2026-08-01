#include "../include/gdi32_api.h"

static uint32_t g_region_counter = 1;

HRGN HRGN_CreateRectRgn(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    (void)x1; (void)y1; (void)x2; (void)y2;
    return (HRGN)(g_region_counter++);
}

int32_t CombineRgn(HRGN hrgnDest, HRGN hrgnSrc1, HRGN hrgnSrc2, int32_t fnCombineMode) {
    (void)hrgnDest; (void)hrgnSrc1; (void)hrgnSrc2; (void)fnCombineMode;
    return 1; // SIMPLEREGION
}

int32_t OffsetRgn(HRGN hrgn, int32_t x, int32_t y) {
    (void)hrgn; (void)x; (void)y;
    return 1;
}
