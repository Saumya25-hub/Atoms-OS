/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "third_party/skia/include/core/SkPaint.h"

SkPaint::SkPaint() {
    reset();
}

SkPaint::SkPaint(const SkPaint& other)
    : fColor(other.fColor)
    , fWidth(other.fWidth)
    , fStyle(other.fStyle)
    , fCap(other.fCap)
    , fJoin(other.fJoin)
    , fBlendMode(other.fBlendMode)
    , fAntiAlias(other.fAntiAlias) {}

SkPaint::~SkPaint() {}

SkPaint& SkPaint::operator=(const SkPaint& other) {
    if (this != &other) {
        fColor = other.fColor;
        fWidth = other.fWidth;
        fStyle = other.fStyle;
        fCap = other.fCap;
        fJoin = other.fJoin;
        fBlendMode = other.fBlendMode;
        fAntiAlias = other.fAntiAlias;
    }
    return *this;
}

void SkPaint::reset() {
    fColor = SK_ColorBLACK;
    fWidth = 1.0f;
    fStyle = kFill_Style;
    fCap = kButt_Cap;
    fJoin = kMiter_Join;
    fBlendMode = SkBlendMode::kSrcOver;
    fAntiAlias = false;
}
