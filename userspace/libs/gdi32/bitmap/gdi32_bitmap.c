#include "../include/gdi32_api.h"

static uint32_t g_bitmap_counter = 1;

HBITMAP CreateBitmap(int32_t nWidth, int32_t nHeight, uint32_t nPlanes, uint32_t nBitCount, const void* lpBits) {
    (void)nWidth; (void)nHeight; (void)nPlanes; (void)nBitCount; (void)lpBits;
    return (HBITMAP)(g_bitmap_counter++);
}

HBITMAP CreateCompatibleBitmap(HDC hdc, int32_t nWidth, int32_t nHeight) {
    (void)hdc;
    return CreateBitmap(nWidth, nHeight, 1, 32, NULL);
}

bool DeleteBitmap(HBITMAP hbm) {
    return (hbm != 0);
}
