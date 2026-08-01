#include "../include/gdi32_api.h"

bool BitBlt(HDC hdcDest, int32_t xDest, int32_t yDest, int32_t w, int32_t h, HDC hdcSrc, int32_t xSrc, int32_t ySrc, uint32_t rop) {
    (void)hdcDest; (void)xDest; (void)yDest; (void)w; (void)h; (void)hdcSrc; (void)xSrc; (void)ySrc; (void)rop;
    return (hdcDest != 0);
}

bool StretchBlt(HDC hdcDest, int32_t xDest, int32_t yDest, int32_t wDest, int32_t hDest, HDC hdcSrc, int32_t xSrc, int32_t ySrc, int32_t wSrc, int32_t hSrc, uint32_t rop) {
    (void)hdcDest; (void)xDest; (void)yDest; (void)wDest; (void)hDest; (void)hdcSrc; (void)xSrc; (void)ySrc; (void)wSrc; (void)hSrc; (void)rop;
    return (hdcDest != 0);
}
