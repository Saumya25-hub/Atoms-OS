/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkCanvas_DEFINED
#define SkCanvas_DEFINED

#include "SkTypes.h"
#include "SkColor.h"
#include "SkRect.h"
#include "SkRRect.h"
#include "SkMatrix.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkImageInfo.h"
#include "SkClipOp.h"
#include "userspace/runtime/cpp/include/vector"

class SkSurface;

class SkCanvas {
public:
    SkCanvas(int width, int height, void* pixels, size_t rowBytes);
    virtual ~SkCanvas();

    int getWidth() const { return fInfo.width(); }
    int getHeight() const { return fInfo.height(); }
    SkImageInfo imageInfo() const { return fInfo; }

    // State Management
    int  save();
    void restore();
    int  getSaveCount() const { return (int)fSavedStates.size() + 1; }

    // Transformation Matrix
    void translate(SkScalar dx, SkScalar dy);
    void scale(SkScalar sx, SkScalar sy);
    void rotate(SkScalar degrees);
    void concat(const SkMatrix& matrix);
    void setMatrix(const SkMatrix& matrix);
    SkMatrix getTotalMatrix() const { return fCurrentMatrix; }

    // Clipping
    void clipRect(const SkRect& rect, SkClipOp op = SkClipOp::kIntersect, bool doAntiAlias = false);
    void clipRRect(const SkRRect& rrect, SkClipOp op = SkClipOp::kIntersect, bool doAntiAlias = false);
    void clipPath(const SkPath& path, SkClipOp op = SkClipOp::kIntersect, bool doAntiAlias = false);
    SkIRect getClipBounds() const { return fCurrentClip; }

    // Drawing Operations
    void clear(SkColor color);
    void drawColor(SkColor color, SkBlendMode mode = SkBlendMode::kSrcOver);
    void drawPaint(const SkPaint& paint);
    void drawRect(const SkRect& rect, const SkPaint& paint);
    void drawIRect(const SkIRect& rect, const SkPaint& paint);
    void drawOval(const SkRect& oval, const SkPaint& paint);
    void drawCircle(SkScalar cx, SkScalar cy, SkScalar radius, const SkPaint& paint);
    void drawRRect(const SkRRect& rrect, const SkPaint& paint);
    void drawLine(SkScalar x0, SkScalar y0, SkScalar x1, SkScalar y1, const SkPaint& paint);
    void drawPath(const SkPath& path, const SkPaint& paint);

    void flush();

private:
    struct State {
        SkMatrix fMatrix;
        SkIRect  fClip;
    };

    SkImageInfo        fInfo;
    uint32_t*          fPixels;
    size_t             fRowBytes;
    SkMatrix           fCurrentMatrix;
    SkIRect            fCurrentClip;
    std::vector<State> fSavedStates;
};

#endif // SkCanvas_DEFINED
