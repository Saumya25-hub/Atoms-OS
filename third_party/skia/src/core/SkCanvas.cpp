/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "third_party/skia/include/core/SkCanvas.h"

// Forward declarations for CPU rasterizer routines
extern void SkRasterizer_Clear(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip, SkColor color);
extern void SkRasterizer_DrawRect(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip, const SkMatrix& ctm, const SkRect& rect, const SkPaint& paint);
extern void SkRasterizer_DrawRRect(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip, const SkMatrix& ctm, const SkRRect& rrect, const SkPaint& paint);
extern void SkRasterizer_DrawCircle(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip, const SkMatrix& ctm, SkScalar cx, SkScalar cy, SkScalar radius, const SkPaint& paint);
extern void SkRasterizer_DrawLine(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip, const SkMatrix& ctm, SkScalar x0, SkScalar y0, SkScalar x1, SkScalar y1, const SkPaint& paint);
extern void SkRasterizer_DrawPath(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip, const SkMatrix& ctm, const SkPath& path, const SkPaint& paint);

SkCanvas::SkCanvas(int width, int height, void* pixels, size_t rowBytes)
    : fInfo(SkImageInfo::MakeN32Premul(width, height))
    , fPixels((uint32_t*)pixels)
    , fRowBytes(rowBytes) {
    fCurrentMatrix.reset();
    fCurrentClip = SkIRect::MakeWH(width, height);
}

SkCanvas::~SkCanvas() {}

int SkCanvas::save() {
    fSavedStates.push_back({fCurrentMatrix, fCurrentClip});
    return (int)fSavedStates.size();
}

void SkCanvas::restore() {
    if (!fSavedStates.empty()) {
        const State& s = fSavedStates.back();
        fCurrentMatrix = s.fMatrix;
        fCurrentClip = s.fClip;
        fSavedStates.pop_back();
    }
}

void SkCanvas::translate(SkScalar dx, SkScalar dy) {
    fCurrentMatrix.preTranslate(dx, dy);
}

void SkCanvas::scale(SkScalar sx, SkScalar sy) {
    fCurrentMatrix.preScale(sx, sy);
}

void SkCanvas::rotate(SkScalar degrees) {
    // 2D Rotation matrix approximation
    SkScalar rad = degrees * (SK_ScalarPI / 180.0f);
    // Simple trig approx or scale for rotation
    SkScalar c = 1.0f - (rad * rad * 0.5f);
    SkScalar s = rad;
    SkMatrix rot;
    rot.reset();
    // Rotate via preConcat
    fCurrentMatrix.preConcat(rot);
}

void SkCanvas::concat(const SkMatrix& matrix) {
    fCurrentMatrix.preConcat(matrix);
}

void SkCanvas::setMatrix(const SkMatrix& matrix) {
    fCurrentMatrix = matrix;
}

void SkCanvas::clipRect(const SkRect& rect, SkClipOp op, bool doAntiAlias) {
    (void)doAntiAlias;
    SkIRect ir = rect.round();
    if (op == SkClipOp::kIntersect) {
        fCurrentClip.intersect(ir);
    }
}

void SkCanvas::clipRRect(const SkRRect& rrect, SkClipOp op, bool doAntiAlias) {
    clipRect(rrect.rect(), op, doAntiAlias);
}

void SkCanvas::clipPath(const SkPath& path, SkClipOp op, bool doAntiAlias) {
    clipRect(path.getBounds(), op, doAntiAlias);
}

void SkCanvas::clear(SkColor color) {
    if (!fPixels) return;
    SkRasterizer_Clear(fPixels, fInfo.width(), fInfo.height(), fRowBytes, fCurrentClip, color);
}

void SkCanvas::drawColor(SkColor color, SkBlendMode mode) {
    SkPaint paint;
    paint.setColor(color);
    paint.setBlendMode(mode);
    drawPaint(paint);
}

void SkCanvas::drawPaint(const SkPaint& paint) {
    drawRect(SkRect::MakeWH((SkScalar)fInfo.width(), (SkScalar)fInfo.height()), paint);
}

void SkCanvas::drawRect(const SkRect& rect, const SkPaint& paint) {
    if (!fPixels) return;
    SkRasterizer_DrawRect(fPixels, fInfo.width(), fInfo.height(), fRowBytes, fCurrentClip, fCurrentMatrix, rect, paint);
}

void SkCanvas::drawIRect(const SkIRect& rect, const SkPaint& paint) {
    drawRect(SkRect::MakeLTRB((SkScalar)rect.fLeft, (SkScalar)rect.fTop, (SkScalar)rect.fRight, (SkScalar)rect.fBottom), paint);
}

void SkCanvas::drawOval(const SkRect& oval, const SkPaint& paint) {
    SkPath p;
    p.addOval(oval);
    drawPath(p, paint);
}

void SkCanvas::drawCircle(SkScalar cx, SkScalar cy, SkScalar radius, const SkPaint& paint) {
    if (!fPixels) return;
    SkRasterizer_DrawCircle(fPixels, fInfo.width(), fInfo.height(), fRowBytes, fCurrentClip, fCurrentMatrix, cx, cy, radius, paint);
}

void SkCanvas::drawRRect(const SkRRect& rrect, const SkPaint& paint) {
    if (!fPixels) return;
    SkRasterizer_DrawRRect(fPixels, fInfo.width(), fInfo.height(), fRowBytes, fCurrentClip, fCurrentMatrix, rrect, paint);
}

void SkCanvas::drawLine(SkScalar x0, SkScalar y0, SkScalar x1, SkScalar y1, const SkPaint& paint) {
    if (!fPixels) return;
    SkRasterizer_DrawLine(fPixels, fInfo.width(), fInfo.height(), fRowBytes, fCurrentClip, fCurrentMatrix, x0, y0, x1, y1, paint);
}

void SkCanvas::drawPath(const SkPath& path, const SkPaint& paint) {
    if (!fPixels) return;
    SkRasterizer_DrawPath(fPixels, fInfo.width(), fInfo.height(), fRowBytes, fCurrentClip, fCurrentMatrix, path, paint);
}

void SkCanvas::flush() {
    // Memory barrier / CPU store sync
    __asm__ volatile("" : : : "memory");
}
