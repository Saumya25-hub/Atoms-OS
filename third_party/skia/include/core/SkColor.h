/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkColor_DEFINED
#define SkColor_DEFINED

#include "SkTypes.h"

typedef uint8_t  SkAlpha;
typedef uint32_t SkColor;
typedef uint32_t SkPMColor;

#define SK_ColorTRANSPARENT     0x00000000
#define SK_ColorBLACK           0xFF000000
#define SK_ColorWHITE           0xFFFFFFFF
#define SK_ColorRED             0xFFFF0000
#define SK_ColorGREEN           0xFF00FF00
#define SK_ColorBLUE            0xFF0000FF
#define SK_ColorYELLOW          0xFFFFFF00
#define SK_ColorCYAN            0xFF00FFFF
#define SK_ColorMAGENTA         0xFFFF00FF

static inline SkColor SkColorSetARGB(SkAlpha a, uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

#define SkColorGetA(color)      (((color) >> 24) & 0xFF)
#define SkColorGetR(color)      (((color) >> 16) & 0xFF)
#define SkColorGetG(color)      (((color) >> 8) & 0xFF)
#define SkColorGetB(color)      (((color) >> 0) & 0xFF)

static inline SkPMColor SkPreMultiplyARGB(SkAlpha a, uint8_t r, uint8_t g, uint8_t b) {
    if (a == 255) {
        return SkColorSetARGB(255, r, g, b);
    }
    if (a == 0) {
        return 0;
    }
    uint32_t scale = a + 1;
    uint32_t pr = (r * scale) >> 8;
    uint32_t pg = (g * scale) >> 8;
    uint32_t pb = (b * scale) >> 8;
    return ((uint32_t)a << 24) | (pr << 16) | (pg << 8) | pb;
}

static inline SkPMColor SkPreMultiplyColor(SkColor c) {
    return SkPreMultiplyARGB(SkColorGetA(c), SkColorGetR(c), SkColorGetG(c), SkColorGetB(c));
}

// Alpha blend helper
static inline uint32_t SkAlphaBlend(uint32_t src, uint32_t dst) {
    uint32_t sa = SkColorGetA(src);
    if (sa == 255) return src;
    if (sa == 0) return dst;

    uint32_t inv_a = 255 - sa;
    uint32_t da = SkColorGetA(dst);
    uint32_t out_a = sa + ((da * inv_a) >> 8);

    uint32_t sr = SkColorGetR(src);
    uint32_t sg = SkColorGetG(src);
    uint32_t sb = SkColorGetB(src);

    uint32_t dr = SkColorGetR(dst);
    uint32_t dg = SkColorGetG(dst);
    uint32_t db = SkColorGetB(dst);

    uint32_t out_r = sr + ((dr * inv_a) >> 8);
    uint32_t out_g = sg + ((dg * inv_a) >> 8);
    uint32_t out_b = sb + ((db * inv_a) >> 8);

    if (out_r > 255) out_r = 255;
    if (out_g > 255) out_g = 255;
    if (out_b > 255) out_b = 255;
    if (out_a > 255) out_a = 255;

    return (out_a << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

#endif // SkColor_DEFINED
