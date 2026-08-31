/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkRect_DEFINED
#define SkRect_DEFINED

#include "SkTypes.h"

struct SkIRect {
    int32_t fLeft;
    int32_t fTop;
    int32_t fRight;
    int32_t fBottom;

    static constexpr SkIRect MakeEmpty() { return {0, 0, 0, 0}; }
    static constexpr SkIRect MakeWH(int32_t w, int32_t h) { return {0, 0, w, h}; }
    static constexpr SkIRect MakeLTRB(int32_t l, int32_t t, int32_t r, int32_t b) { return {l, t, r, b}; }
    static constexpr SkIRect MakeXYWH(int32_t x, int32_t y, int32_t w, int32_t h) { return {x, y, x + w, y + h}; }

    int32_t x() const { return fLeft; }
    int32_t y() const { return fTop; }
    int32_t left() const { return fLeft; }
    int32_t top() const { return fTop; }
    int32_t right() const { return fRight; }
    int32_t bottom() const { return fBottom; }
    int32_t width() const { return fRight - fLeft; }
    int32_t height() const { return fBottom - fTop; }
    bool isEmpty() const { return fLeft >= fRight || fTop >= fBottom; }

    bool intersect(const SkIRect& r) {
        int32_t l = SkScalarMax((SkScalar)fLeft, (SkScalar)r.fLeft);
        int32_t t = SkScalarMax((SkScalar)fTop, (SkScalar)r.fTop);
        int32_t ri = SkScalarMin((SkScalar)fRight, (SkScalar)r.fRight);
        int32_t b = SkScalarMin((SkScalar)fBottom, (SkScalar)r.fBottom);
        if (l < ri && t < b) {
            fLeft = l; fTop = t; fRight = ri; fBottom = b;
            return true;
        }
        return false;
    }
};

struct SkRect {
    SkScalar fLeft;
    SkScalar fTop;
    SkScalar fRight;
    SkScalar fBottom;

    static constexpr SkRect MakeEmpty() { return {0, 0, 0, 0}; }
    static constexpr SkRect MakeWH(SkScalar w, SkScalar h) { return {0, 0, w, h}; }
    static constexpr SkRect MakeLTRB(SkScalar l, SkScalar t, SkScalar r, SkScalar b) { return {l, t, r, b}; }
    static constexpr SkRect MakeXYWH(SkScalar x, SkScalar y, SkScalar w, SkScalar h) { return {x, y, x + w, y + h}; }

    SkScalar x() const { return fLeft; }
    SkScalar y() const { return fTop; }
    SkScalar left() const { return fLeft; }
    SkScalar top() const { return fTop; }
    SkScalar right() const { return fRight; }
    SkScalar bottom() const { return fBottom; }
    SkScalar width() const { return fRight - fLeft; }
    SkScalar height() const { return fBottom - fTop; }
    bool isEmpty() const { return fLeft >= fRight || fTop >= fBottom; }

    SkIRect round() const {
        return SkIRect::MakeLTRB(SkScalarRoundToInt(fLeft), SkScalarRoundToInt(fTop),
                                 SkScalarRoundToInt(fRight), SkScalarRoundToInt(fBottom));
    }

    SkIRect roundOut() const {
        return SkIRect::MakeLTRB(SkScalarFloorToInt(fLeft), SkScalarFloorToInt(fTop),
                                 SkScalarCeilToInt(fRight), SkScalarCeilToInt(fBottom));
    }

    bool contains(SkScalar x, SkScalar y) const {
        return x >= fLeft && x < fRight && y >= fTop && y < fBottom;
    }
};

#endif // SkRect_DEFINED
