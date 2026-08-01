#include "../include/gdi32_api.h"

bool TransparentBlt(HDC hdcDest, int32_t xDest, int32_t yDest, int32_t wDest, int32_t hDest, HDC hdcSrc, int32_t xSrc, int32_t ySrc, int32_t wSrc, int32_t hSrc, uint32_t crTransparent) {
    (void)hdcDest; (void)xDest; (void)yDest; (void)wDest; (void)hDest; (void)hdcSrc; (void)xSrc; (void)ySrc; (void)wSrc; (void)hSrc; (void)crTransparent;
    return (hdcDest != 0);
}

bool AlphaBlend(HDC hdcDest, int32_t xDest, int32_t yDest, int32_t wDest, int32_t hDest, HDC hdcSrc, int32_t xSrc, int32_t ySrc, int32_t wSrc, int32_t hSrc, BLENDFUNCTION ft) {
    (void)hdcDest; (void)xDest; (void)yDest; (void)wDest; (void)hDest; (void)hdcSrc; (void)xSrc; (void)ySrc; (void)wSrc; (void)hSrc; (void)ft;
    return (hdcDest != 0);
}
