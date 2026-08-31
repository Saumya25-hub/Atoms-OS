/*
 * Copyright 2012 Google Inc.
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkRRect_DEFINED
#define SkRRect_DEFINED

#include "SkRect.h"

class SkRRect {
public:
    enum Type {
        kEmpty_Type,
        kRect_Type,
        kOval_Type,
        kSimple_Type,
        kNinePatch_Type,
        kComplex_Type
    };

    SkRRect() : fType(kEmpty_Type), fRect(SkRect::MakeEmpty()), fRadX(0), fRadY(0) {}

    static SkRRect MakeEmpty() { return SkRRect(); }
    static SkRRect MakeRect(const SkRect& r) {
        SkRRect rr;
        rr.fType = kRect_Type;
        rr.fRect = r;
        rr.fRadX = rr.fRadY = 0;
        return rr;
    }
    static SkRRect MakeOval(const SkRect& r) {
        SkRRect rr;
        rr.fType = kOval_Type;
        rr.fRect = r;
        rr.fRadX = r.width() * SK_ScalarHalf;
        rr.fRadY = r.height() * SK_ScalarHalf;
        return rr;
    }
    static SkRRect MakeRectXY(const SkRect& r, SkScalar xRad, SkScalar yRad) {
        SkRRect rr;
        rr.setRectXY(r, xRad, yRad);
        return rr;
    }

    void setRectXY(const SkRect& r, SkScalar xRad, SkScalar yRad) {
        fRect = r;
        if (xRad <= 0 || yRad <= 0) {
            fType = kRect_Type;
            fRadX = fRadY = 0;
        } else {
            fType = kSimple_Type;
            fRadX = xRad;
            fRadY = yRad;
        }
    }

    const SkRect& rect() const { return fRect; }
    SkScalar width() const { return fRect.width(); }
    SkScalar height() const { return fRect.height(); }
    SkScalar getSimpleRadiiX() const { return fRadX; }
    SkScalar getSimpleRadiiY() const { return fRadY; }
    Type getType() const { return fType; }

    bool contains(SkScalar x, SkScalar y) const;

private:
    Type     fType;
    SkRect   fRect;
    SkScalar fRadX;
    SkScalar fRadY;
};

#endif // SkRRect_DEFINED
