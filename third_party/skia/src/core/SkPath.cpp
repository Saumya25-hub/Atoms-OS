/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "third_party/skia/include/core/SkPath.h"

SkPath::SkPath() : fFillType(kWinding_FillType) {}

SkPath::SkPath(const SkPath& other)
    : fVerbs(other.fVerbs)
    , fPts(other.fPts)
    , fFillType(other.fFillType) {}

SkPath::~SkPath() {}

SkPath& SkPath::operator=(const SkPath& other) {
    if (this != &other) {
        fVerbs = other.fVerbs;
        fPts = other.fPts;
        fFillType = other.fFillType;
    }
    return *this;
}

void SkPath::reset() {
    fVerbs.clear();
    fPts.clear();
    fFillType = kWinding_FillType;
}

void SkPath::rewind() {
    reset();
}

SkPath& SkPath::moveTo(SkScalar x, SkScalar y) {
    fVerbs.push_back(kMove_Verb);
    fPts.push_back({x, y});
    return *this;
}

SkPath& SkPath::lineTo(SkScalar x, SkScalar y) {
    fVerbs.push_back(kLine_Verb);
    fPts.push_back({x, y});
    return *this;
}

SkPath& SkPath::quadTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2) {
    fVerbs.push_back(kQuad_Verb);
    fPts.push_back({x1, y1});
    fPts.push_back({x2, y2});
    return *this;
}

SkPath& SkPath::cubicTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2, SkScalar x3, SkScalar y3) {
    fVerbs.push_back(kCubic_Verb);
    fPts.push_back({x1, y1});
    fPts.push_back({x2, y2});
    fPts.push_back({x3, y3});
    return *this;
}

SkPath& SkPath::close() {
    fVerbs.push_back(kClose_Verb);
    return *this;
}

SkPath& SkPath::addRect(const SkRect& rect) {
    moveTo(rect.fLeft, rect.fTop);
    lineTo(rect.fRight, rect.fTop);
    lineTo(rect.fRight, rect.fBottom);
    lineTo(rect.fLeft, rect.fBottom);
    close();
    return *this;
}

SkPath& SkPath::addOval(const SkRect& oval) {
    SkScalar cx = oval.fLeft + oval.width() * SK_ScalarHalf;
    SkScalar cy = oval.fTop + oval.height() * SK_ScalarHalf;
    SkScalar rx = oval.width() * SK_ScalarHalf;
    SkScalar ry = oval.height() * SK_ScalarHalf;

    // Approximate oval with 4 cubic bezier segments
    const SkScalar k = 0.5522847498f;
    SkScalar kx = rx * k;
    SkScalar ky = ry * k;

    moveTo(cx + rx, cy);
    cubicTo(cx + rx, cy + ky, cx + kx, cy + ry, cx, cy + ry);
    cubicTo(cx - kx, cy + ry, cx - rx, cy + ky, cx - rx, cy);
    cubicTo(cx - rx, cy - ky, cx - kx, cy - ry, cx, cy - ry);
    cubicTo(cx + kx, cy - ry, cx + rx, cy - ky, cx + rx, cy);
    close();
    return *this;
}

SkPath& SkPath::addCircle(SkScalar x, SkScalar y, SkScalar radius) {
    return addOval(SkRect::MakeLTRB(x - radius, y - radius, x + radius, y + radius));
}

SkPath& SkPath::addRRect(const SkRRect& rrect) {
    const SkRect& r = rrect.rect();
    SkScalar rx = rrect.getSimpleRadiiX();
    SkScalar ry = rrect.getSimpleRadiiY();

    if (rx <= 0 || ry <= 0) {
        return addRect(r);
    }

    moveTo(r.fLeft + rx, r.fTop);
    lineTo(r.fRight - rx, r.fTop);
    quadTo(r.fRight, r.fTop, r.fRight, r.fTop + ry);
    lineTo(r.fRight, r.fBottom - ry);
    quadTo(r.fRight, r.fBottom, r.fRight - rx, r.fBottom);
    lineTo(r.fLeft + rx, r.fBottom);
    quadTo(r.fLeft, r.fBottom, r.fLeft, r.fBottom - ry);
    lineTo(r.fLeft, r.fTop + ry);
    quadTo(r.fLeft, r.fTop, r.fLeft + rx, r.fTop);
    close();
    return *this;
}

SkRect SkPath::getBounds() const {
    if (fPts.empty()) return SkRect::MakeEmpty();
    SkScalar minX = fPts[0].fX, maxX = fPts[0].fX;
    SkScalar minY = fPts[0].fY, maxY = fPts[0].fY;
    for (size_t i = 1; i < fPts.size(); i++) {
        if (fPts[i].fX < minX) minX = fPts[i].fX;
        if (fPts[i].fX > maxX) maxX = fPts[i].fX;
        if (fPts[i].fY < minY) minY = fPts[i].fY;
        if (fPts[i].fY > maxY) maxY = fPts[i].fY;
    }
    return SkRect::MakeLTRB(minX, minY, maxX, maxY);
}
