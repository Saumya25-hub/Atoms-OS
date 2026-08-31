/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkPath_DEFINED
#define SkPath_DEFINED

#include "SkRect.h"
#include "SkRRect.h"
#include "SkMatrix.h"
#include "userspace/runtime/cpp/include/vector"

class SkPath {
public:
    enum Verb {
        kMove_Verb,
        kLine_Verb,
        kQuad_Verb,
        kCubic_Verb,
        kClose_Verb,
        kDone_Verb
    };

    enum FillType {
        kWinding_FillType,
        kEvenOdd_FillType,
        kInverseWinding_FillType,
        kInverseEvenOdd_FillType
    };

    struct Point {
        SkScalar fX;
        SkScalar fY;
    };

    SkPath();
    SkPath(const SkPath&);
    ~SkPath();
    SkPath& operator=(const SkPath&);

    void reset();
    void rewind();

    bool isEmpty() const { return fVerbs.empty(); }

    FillType getFillType() const { return fFillType; }
    void setFillType(FillType ft) { fFillType = ft; }

    SkPath& moveTo(SkScalar x, SkScalar y);
    SkPath& lineTo(SkScalar x, SkScalar y);
    SkPath& quadTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2);
    SkPath& cubicTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2, SkScalar x3, SkScalar y3);
    SkPath& close();

    SkPath& addRect(const SkRect& rect);
    SkPath& addOval(const SkRect& oval);
    SkPath& addCircle(SkScalar x, SkScalar y, SkScalar radius);
    SkPath& addRRect(const SkRRect& rrect);

    SkRect getBounds() const;

    // Verb iteration for rasterizer
    size_t countVerbs() const { return fVerbs.size(); }
    size_t countPoints() const { return fPts.size(); }
    const Verb* verbs() const { return fVerbs.data(); }
    const Point* points() const { return fPts.data(); }

private:
    std::vector<Verb>  fVerbs;
    std::vector<Point> fPts;
    FillType           fFillType;
};

#endif // SkPath_DEFINED
