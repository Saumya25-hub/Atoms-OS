/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkMatrix_DEFINED
#define SkMatrix_DEFINED

#include "SkRect.h"

class SkMatrix {
public:
    enum {
        kMScaleX = 0,
        kMSkewX  = 1,
        kMTransX = 2,
        kMSkewY  = 3,
        kMScaleY = 4,
        kMTransY = 5,
        kMPersp0 = 6,
        kMPersp1 = 7,
        kMPersp2 = 8,
    };

    SkMatrix() { reset(); }

    void reset() {
        fMat[kMScaleX] = 1; fMat[kMSkewX]  = 0; fMat[kMTransX] = 0;
        fMat[kMSkewY]  = 0; fMat[kMScaleY] = 1; fMat[kMTransY] = 0;
        fMat[kMPersp0] = 0; fMat[kMPersp1] = 0; fMat[kMPersp2] = 1;
    }

    static SkMatrix I() {
        SkMatrix m;
        m.reset();
        return m;
    }

    static SkMatrix Translate(SkScalar dx, SkScalar dy) {
        SkMatrix m;
        m.setTranslate(dx, dy);
        return m;
    }

    static SkMatrix Scale(SkScalar sx, SkScalar sy) {
        SkMatrix m;
        m.setScale(sx, sy);
        return m;
    }

    void setTranslate(SkScalar dx, SkScalar dy) {
        fMat[kMScaleX] = 1; fMat[kMSkewX]  = 0; fMat[kMTransX] = dx;
        fMat[kMSkewY]  = 0; fMat[kMScaleY] = 1; fMat[kMTransY] = dy;
        fMat[kMPersp0] = 0; fMat[kMPersp1] = 0; fMat[kMPersp2] = 1;
    }

    void setScale(SkScalar sx, SkScalar sy) {
        fMat[kMScaleX] = sx; fMat[kMSkewX]  = 0;  fMat[kMTransX] = 0;
        fMat[kMSkewY]  = 0;  fMat[kMScaleY] = sy; fMat[kMTransY] = 0;
        fMat[kMPersp0] = 0;  fMat[kMPersp1] = 0;  fMat[kMPersp2] = 1;
    }

    void preTranslate(SkScalar dx, SkScalar dy);
    void postTranslate(SkScalar dx, SkScalar dy);
    void preScale(SkScalar sx, SkScalar sy);
    void postScale(SkScalar sx, SkScalar sy);
    void preConcat(const SkMatrix& other);
    void postConcat(const SkMatrix& other);

    void mapPoints(SkScalar* dstX, SkScalar* dstY, SkScalar srcX, SkScalar srcY) const {
        *dstX = fMat[kMScaleX] * srcX + fMat[kMSkewX] * srcY + fMat[kMTransX];
        *dstY = fMat[kMSkewY] * srcX + fMat[kMScaleY] * srcY + fMat[kMTransY];
    }

    SkScalar get(int index) const { return fMat[index]; }
    SkScalar getTranslateX() const { return fMat[kMTransX]; }
    SkScalar getTranslateY() const { return fMat[kMTransY]; }
    SkScalar getScaleX() const { return fMat[kMScaleX]; }
    SkScalar getScaleY() const { return fMat[kMScaleY]; }

private:
    SkScalar fMat[9];
};

#endif // SkMatrix_DEFINED
