#ifndef BOS_GDI32_TYPES_H
#define BOS_GDI32_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "userspace/libs/user32/include/user32_types.h"

typedef uint32_t HDC;
typedef uint32_t HGDIOBJ;
typedef uint32_t HBITMAP;
typedef uint32_t HFONT;
typedef uint32_t HPEN;
typedef uint32_t HBRUSH;
typedef uint32_t HRGN;
typedef uint32_t COLORREF;
typedef uint32_t HPAINTBUFFER;

#define RGB(r,g,b)          ((COLORREF)(((uint8_t)(r)|((uint16_t)((uint8_t)(g))<<8))|(((uint32_t)(uint8_t)(b))<<16)))
#define GetRValue(rgb)      ((uint8_t)(rgb))
#define GetGValue(rgb)      ((uint8_t)(((uint16_t)(rgb)) >> 8))
#define GetBValue(rgb)      ((uint8_t)((rgb) >> 16))

#define PS_SOLID            0
#define PS_DASH             1
#define PS_DOT              2
#define PS_NULL             5

#define RGN_AND             1
#define RGN_OR              2
#define RGN_XOR             3
#define RGN_DIFF            4
#define RGN_COPY            5

#define SRCCOPY             0x00CC0020
#define SRCPAINT            0x00EE0086
#define SRCAND              0x008800C6
#define SRCINVERT           0x00660046

#define WHITE_BRUSH         0
#define LTGRAY_BRUSH        1
#define GRAY_BRUSH          2
#define DKGRAY_BRUSH        3
#define BLACK_BRUSH         4
#define NULL_BRUSH          5
#define WHITE_PEN           6
#define BLACK_PEN           7
#define NULL_PEN            8
#define SYSTEM_FONT         10
#define DEFAULT_GUI_FONT    17

typedef struct {
    int32_t cx;
    int32_t cy;
} SIZE, *PSIZE, *LPSIZE;

typedef struct {
    int32_t  bmType;
    int32_t  bmWidth;
    int32_t  bmHeight;
    int32_t  bmWidthBytes;
    uint16_t bmPlanes;
    uint16_t bmBitsPixel;
    void*    bmBits;
} BITMAP, *PBITMAP, *LPBITMAP;

typedef struct {
    int32_t  lfHeight;
    int32_t  lfWidth;
    int32_t  lfEscapement;
    int32_t  lfOrientation;
    int32_t  lfWeight;
    uint8_t  lfItalic;
    uint8_t  lfUnderline;
    uint8_t  lfStrikeOut;
    uint8_t  lfCharSet;
    uint8_t  lfOutPrecision;
    uint8_t  lfClipPrecision;
    uint8_t  lfQuality;
    uint8_t  lfPitchAndFamily;
    char     lfFaceName[32];
} LOGFONT, *PLOGFONT, *LPLOGFONT;

typedef struct {
    int32_t tmHeight;
    int32_t tmAscent;
    int32_t tmDescent;
    int32_t tmInternalLeading;
    int32_t tmExternalLeading;
    int32_t tmAveCharWidth;
    int32_t tmMaxCharWidth;
    int32_t tmWeight;
    int32_t tmOverhang;
    int32_t tmDigitizedAspectX;
    int32_t tmDigitizedAspectY;
    uint8_t tmFirstChar;
    uint8_t tmLastChar;
    uint8_t tmDefaultChar;
    uint8_t tmBreakChar;
    uint8_t tmItalic;
    uint8_t tmUnderlined;
    uint8_t tmStruckOut;
    uint8_t tmPitchAndFamily;
    uint8_t tmCharSet;
} TEXTMETRIC, *PTEXTMETRIC, *LPTEXTMETRIC;

typedef struct {
    uint8_t BlendOp;
    uint8_t BlendFlags;
    uint8_t SourceConstantAlpha;
    uint8_t AlphaFormat;
} BLENDFUNCTION;

#define AC_SRC_OVER         0x00
#define AC_SRC_ALPHA        0x01

#endif // BOS_GDI32_TYPES_H
