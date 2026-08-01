#ifndef BOS_GDI32_API_H
#define BOS_GDI32_API_H

#include "gdi32_types.h"

int32_t GDI32_Init(void);
int32_t GDI32_Shutdown(void);

HDC  CreateCompatibleDC(HDC hdc);
bool DeleteDC(HDC hdc);
int32_t SaveDC(HDC hdc);
bool RestoreDC(HDC hdc, int32_t nSavedDC);

HGDIOBJ SelectObject(HDC hdc, HGDIOBJ hso);
bool    DeleteObject(HGDIOBJ hObject);
HGDIOBJ GetStockObject(int32_t fnObject);

HPEN   CreatePen(int32_t iStyle, int32_t cWidth, COLORREF color);
HBRUSH CreateSolidBrush(COLORREF color);
HBRUSH CreatePatternBrush(HBITMAP hbm);

HFONT DeleteFont(HFONT hfont);
HFONT CreateFont(int32_t cHeight, int32_t cWidth, int32_t cEscapement, int32_t cOrientation, int32_t cWeight, uint32_t bItalic, uint32_t bUnderline, uint32_t bStrikeOut, uint32_t iCharSet, uint32_t iOutPrecision, uint32_t iClipPrecision, uint32_t iQuality, uint32_t iPitchAndFamily, const char* pszFaceName);
HFONT CreateFontIndirect(const LOGFONT* lplf);

bool TextOut(HDC hdc, int32_t x, int32_t y, const char* lpString, int32_t c);
int32_t DrawText(HDC hdc, const char* lpchText, int32_t cchText, RECT* lprc, uint32_t format);
bool ExtTextOut(HDC hdc, int32_t x, int32_t y, uint32_t options, const RECT* lprect, const char* lpString, uint32_t c, const int32_t* lpDx);
COLORREF SetTextColor(HDC hdc, COLORREF color);
COLORREF SetBkColor(HDC hdc, COLORREF color);

bool MoveToEx(HDC hdc, int32_t x, int32_t y, POINT* lppt);
bool LineTo(HDC hdc, int32_t x, int32_t y);
bool Rectangle(HDC hdc, int32_t left, int32_t top, int32_t right, int32_t bottom);
bool RoundRect(HDC hdc, int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height);
bool Ellipse(HDC hdc, int32_t left, int32_t top, int32_t right, int32_t bottom);
bool Polygon(HDC hdc, const POINT* apt, int32_t cpt);
bool Polyline(HDC hdc, const POINT* apt, int32_t cpt);
bool Arc(HDC hdc, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t x4, int32_t y4);
bool Pie(HDC hdc, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t x4, int32_t y4);
bool Chord(HDC hdc, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t x4, int32_t y4);

int32_t FillRect(HDC hdc, const RECT* lprc, HBRUSH hbr);
int32_t FrameRect(HDC hdc, const RECT* lprc, HBRUSH hbr);
bool InvertRect(HDC hdc, const RECT* lprc);
bool DrawFocusRect(HDC hdc, const RECT* lprc);

HBITMAP CreateBitmap(int32_t nWidth, int32_t nHeight, uint32_t nPlanes, uint32_t nBitCount, const void* lpBits);
HBITMAP CreateCompatibleBitmap(HDC hdc, int32_t nWidth, int32_t nHeight);
bool    DeleteBitmap(HBITMAP hbm);

bool BitBlt(HDC hdcDest, int32_t xDest, int32_t yDest, int32_t w, int32_t h, HDC hdcSrc, int32_t xSrc, int32_t ySrc, uint32_t rop);
bool StretchBlt(HDC hdcDest, int32_t xDest, int32_t yDest, int32_t wDest, int32_t hDest, HDC hdcSrc, int32_t xSrc, int32_t ySrc, int32_t wSrc, int32_t hSrc, uint32_t rop);
bool TransparentBlt(HDC hdcDest, int32_t xDest, int32_t yDest, int32_t wDest, int32_t hDest, HDC hdcSrc, int32_t xSrc, int32_t ySrc, int32_t wSrc, int32_t hSrc, uint32_t crTransparent);
bool AlphaBlend(HDC hdcDest, int32_t xDest, int32_t yDest, int32_t wDest, int32_t hDest, HDC hdcSrc, int32_t xSrc, int32_t ySrc, int32_t wSrc, int32_t hSrc, BLENDFUNCTION ft);

HRGN HRGN_CreateRectRgn(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
int32_t CombineRgn(HRGN hrgnDest, HRGN hrgnSrc1, HRGN hrgnSrc2, int32_t fnCombineMode);
int32_t OffsetRgn(HRGN hrgn, int32_t x, int32_t y);
int32_t SelectClipRgn(HDC hdc, HRGN hrgn);
int32_t IntersectClipRect(HDC hdc, int32_t left, int32_t top, int32_t right, int32_t bottom);

COLORREF SetPixel(HDC hdc, int32_t x, int32_t y, COLORREF color);
COLORREF GetPixel(HDC hdc, int32_t x, int32_t y);

HPAINTBUFFER BeginBufferedPaint(HDC hdcTarget, const RECT* prcTarget, uint32_t dwFormat, void* pPaintParams, HDC* phdcTargetOut);
bool          EndBufferedPaint(HPAINTBUFFER hBufferedPaint, bool fUpdateTarget);

void gdi32_run_certification_suite(void);

#endif // BOS_GDI32_API_H
