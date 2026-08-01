#include "../include/gdi32_api.h"

bool Rectangle(HDC hdc, int32_t left, int32_t top, int32_t right, int32_t bottom) {
    (void)hdc; (void)left; (void)top; (void)right; (void)bottom;
    return (hdc != 0);
}

bool RoundRect(HDC hdc, int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height) {
    (void)hdc; (void)left; (void)top; (void)right; (void)bottom; (void)width; (void)height;
    return (hdc != 0);
}

bool Ellipse(HDC hdc, int32_t left, int32_t top, int32_t right, int32_t bottom) {
    (void)hdc; (void)left; (void)top; (void)right; (void)bottom;
    return (hdc != 0);
}

bool Polygon(HDC hdc, const POINT* apt, int32_t cpt) {
    (void)hdc; (void)apt; (void)cpt;
    return (hdc != 0);
}

bool Polyline(HDC hdc, const POINT* apt, int32_t cpt) {
    (void)hdc; (void)apt; (void)cpt;
    return (hdc != 0);
}

bool Arc(HDC hdc, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t x4, int32_t y4) {
    (void)hdc; (void)x1; (void)y1; (void)x2; (void)y2; (void)x3; (void)y3; (void)x4; (void)y4;
    return (hdc != 0);
}

bool Pie(HDC hdc, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t x4, int32_t y4) {
    (void)hdc; (void)x1; (void)y1; (void)x2; (void)y2; (void)x3; (void)y3; (void)x4; (void)y4;
    return (hdc != 0);
}

bool Chord(HDC hdc, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t x4, int32_t y4) {
    (void)hdc; (void)x1; (void)y1; (void)x2; (void)y2; (void)x3; (void)y3; (void)x4; (void)y4;
    return (hdc != 0);
}

int32_t FillRect(HDC hdc, const RECT* lprc, HBRUSH hbr) {
    (void)hdc; (void)lprc; (void)hbr;
    return (hdc != 0) ? 1 : 0;
}

int32_t FrameRect(HDC hdc, const RECT* lprc, HBRUSH hbr) {
    (void)hdc; (void)lprc; (void)hbr;
    return (hdc != 0) ? 1 : 0;
}

bool InvertRect(HDC hdc, const RECT* lprc) {
    (void)hdc; (void)lprc;
    return (hdc != 0);
}

bool DrawFocusRect(HDC hdc, const RECT* lprc) {
    (void)hdc; (void)lprc;
    return (hdc != 0);
}

HPAINTBUFFER BeginBufferedPaint(HDC hdcTarget, const RECT* prcTarget, uint32_t dwFormat, void* pPaintParams, HDC* phdcTargetOut) {
    (void)prcTarget; (void)dwFormat; (void)pPaintParams;
    if (phdcTargetOut) *phdcTargetOut = hdcTarget;
    return (HPAINTBUFFER)1;
}

bool EndBufferedPaint(HPAINTBUFFER hBufferedPaint, bool fUpdateTarget) {
    (void)hBufferedPaint; (void)fUpdateTarget;
    return true;
}
