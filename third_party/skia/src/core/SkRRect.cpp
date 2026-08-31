/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkRRect.h"

bool SkRRect::contains(SkScalar x, SkScalar y) const {
    if (!fRect.contains(x, y)) return false;
    if (fType == kRect_Type) return true;

    // Check corners for rounded rect
    SkScalar dx = 0;
    SkScalar dy = 0;

    if (x < fRect.fLeft + fRadX && y < fRect.fTop + fRadY) {
        // Top-Left corner
        dx = (x - (fRect.fLeft + fRadX)) / fRadX;
        dy = (y - (fRect.fTop + fRadY)) / fRadY;
    } else if (x > fRect.fRight - fRadX && y < fRect.fTop + fRadY) {
        // Top-Right corner
        dx = (x - (fRect.fRight - fRadX)) / fRadX;
        dy = (y - (fRect.fTop + fRadY)) / fRadY;
    } else if (x < fRect.fLeft + fRadX && y > fRect.fBottom - fRadY) {
        // Bottom-Left corner
        dx = (x - (fRect.fLeft + fRadX)) / fRadX;
        dy = (y - (fRect.fBottom - fRadY)) / fRadY;
    } else if (x > fRect.fRight - fRadX && y > fRect.fBottom - fRadY) {
        // Bottom-Right corner
        dx = (x - (fRect.fRight - fRadX)) / fRadX;
        dy = (y - (fRect.fBottom - fRadY)) / fRadY;
    } else {
        return true;
    }

    return (dx * dx + dy * dy) <= 1.0f;
}
