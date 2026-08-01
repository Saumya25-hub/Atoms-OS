#include "../include/gdi32_api.h"

int32_t SelectClipRgn(HDC hdc, HRGN hrgn) {
    (void)hdc; (void)hrgn;
    return 1;
}

int32_t IntersectClipRect(HDC hdc, int32_t left, int32_t top, int32_t right, int32_t bottom) {
    (void)hdc; (void)left; (void)top; (void)right; (void)bottom;
    return 1;
}
