/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "third_party/skia/include/core/SkMatrix.h"

void SkMatrix::preTranslate(SkScalar dx, SkScalar dy) {
    SkMatrix m;
    m.setTranslate(dx, dy);
    preConcat(m);
}

void SkMatrix::postTranslate(SkScalar dx, SkScalar dy) {
    SkMatrix m;
    m.setTranslate(dx, dy);
    postConcat(m);
}

void SkMatrix::preScale(SkScalar sx, SkScalar sy) {
    SkMatrix m;
    m.setScale(sx, sy);
    preConcat(m);
}

void SkMatrix::postScale(SkScalar sx, SkScalar sy) {
    SkMatrix m;
    m.setScale(sx, sy);
    postConcat(m);
}

void SkMatrix::preConcat(const SkMatrix& other) {
    SkMatrix result;
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            SkScalar sum = 0;
            for (int k = 0; k < 3; k++) {
                sum += fMat[r * 3 + k] * other.fMat[k * 3 + c];
            }
            result.fMat[r * 3 + c] = sum;
        }
    }
    *this = result;
}

void SkMatrix::postConcat(const SkMatrix& other) {
    SkMatrix result;
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            SkScalar sum = 0;
            for (int k = 0; k < 3; k++) {
                sum += other.fMat[r * 3 + k] * fMat[k * 3 + c];
            }
            result.fMat[r * 3 + c] = sum;
        }
    }
    *this = result;
}
