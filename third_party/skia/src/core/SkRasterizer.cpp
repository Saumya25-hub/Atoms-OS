/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "third_party/skia/include/core/SkTypes.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"
#include "userspace/runtime/c/include/string.h"
#include "userspace/runtime/cpp/include/vector"

// Inline pixel plotting with alpha compositing
static inline void SkRasterizer_PlotPixel(uint32_t* dst, int w, int h, size_t rowBytes,
                                          const SkIRect& clip, int x, int y, uint32_t color) {
    if (x < clip.fLeft || x >= clip.fRight || y < clip.fTop || y >= clip.fBottom) return;
    if (x < 0 || x >= w || y < 0 || y >= h) return;

    size_t stridePixels = rowBytes / sizeof(uint32_t);
    uint32_t* target = dst + (y * stridePixels) + x;
    *target = SkAlphaBlend(color, *target);
}

// 1. Clear
void SkRasterizer_Clear(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip, SkColor color) {
    int l = SkScalarMax((SkScalar)0, (SkScalar)clip.fLeft);
    int t = SkScalarMax((SkScalar)0, (SkScalar)clip.fTop);
    int r = SkScalarMin((SkScalar)w, (SkScalar)clip.fRight);
    int b = SkScalarMin((SkScalar)h, (SkScalar)clip.fBottom);

    if (l >= r || t >= b) return;

    size_t stridePixels = rowBytes / sizeof(uint32_t);
    for (int y = t; y < b; y++) {
        uint32_t* row = dst + (y * stridePixels) + l;
        for (int x = l; x < r; x++) {
            *row++ = color;
        }
    }
}

// 2. Draw Rect
void SkRasterizer_DrawRect(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip,
                           const SkMatrix& ctm, const SkRect& rect, const SkPaint& paint) {
    SkScalar x0, y0, x1, y1;
    ctm.mapPoints(&x0, &y0, rect.fLeft, rect.fTop);
    ctm.mapPoints(&x1, &y1, rect.fRight, rect.fBottom);

    if (x0 > x1) SkTSwap(x0, x1);
    if (y0 > y1) SkTSwap(y0, y1);

    int l = SkScalarFloorToInt(x0);
    int t = SkScalarFloorToInt(y0);
    int r = SkScalarCeilToInt(x1);
    int b = SkScalarCeilToInt(y1);

    l = SkScalarMax((SkScalar)l, (SkScalar)clip.fLeft);
    t = SkScalarMax((SkScalar)t, (SkScalar)clip.fTop);
    r = SkScalarMin((SkScalar)r, (SkScalar)clip.fRight);
    b = SkScalarMin((SkScalar)b, (SkScalar)clip.fBottom);

    if (l >= r || t >= b) return;

    uint32_t color = paint.getColor();
    size_t stridePixels = rowBytes / sizeof(uint32_t);

    if (paint.getStyle() == SkPaint::kFill_Style) {
        for (int y = t; y < b; y++) {
            uint32_t* row = dst + (y * stridePixels) + l;
            for (int x = l; x < r; x++) {
                *row = SkAlphaBlend(color, *row);
                row++;
            }
        }
    } else {
        // Stroke rectangle
        int strokeWidth = SkScalarRoundToInt(paint.getStrokeWidth());
        if (strokeWidth < 1) strokeWidth = 1;

        for (int y = t; y < b; y++) {
            for (int x = l; x < r; x++) {
                if (x < l + strokeWidth || x >= r - strokeWidth ||
                    y < t + strokeWidth || y >= b - strokeWidth) {
                    SkRasterizer_PlotPixel(dst, w, h, rowBytes, clip, x, y, color);
                }
            }
        }
    }
}

// 3. Draw RRect (Rounded Rect)
void SkRasterizer_DrawRRect(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip,
                            const SkMatrix& ctm, const SkRRect& rrect, const SkPaint& paint) {
    SkScalar x0, y0, x1, y1;
    const SkRect& r = rrect.rect();
    ctm.mapPoints(&x0, &y0, r.fLeft, r.fTop);
    ctm.mapPoints(&x1, &y1, r.fRight, r.fBottom);

    if (x0 > x1) SkTSwap(x0, x1);
    if (y0 > y1) SkTSwap(y0, y1);

    int l = SkScalarFloorToInt(x0);
    int t = SkScalarFloorToInt(y0);
    int ri = SkScalarCeilToInt(x1);
    int b = SkScalarCeilToInt(y1);

    l = SkScalarMax((SkScalar)l, (SkScalar)clip.fLeft);
    t = SkScalarMax((SkScalar)t, (SkScalar)clip.fTop);
    ri = SkScalarMin((SkScalar)ri, (SkScalar)clip.fRight);
    b = SkScalarMin((SkScalar)b, (SkScalar)clip.fBottom);

    if (l >= ri || t >= b) return;

    uint32_t color = paint.getColor();
    SkScalar radX = rrect.getSimpleRadiiX() * ctm.getScaleX();
    SkScalar radY = rrect.getSimpleRadiiY() * ctm.getScaleY();

    for (int y = t; y < b; y++) {
        for (int x = l; x < ri; x++) {
            bool inCorner = false;
            SkScalar dx = 0, dy = 0;

            if (x < x0 + radX && y < y0 + radY) {
                dx = (x - (x0 + radX)) / radX;
                dy = (y - (y0 + radY)) / radY;
                inCorner = true;
            } else if (x > x1 - radX && y < y0 + radY) {
                dx = (x - (x1 - radX)) / radX;
                dy = (y - (y0 + radY)) / radY;
                inCorner = true;
            } else if (x < x0 + radX && y > y1 - radY) {
                dx = (x - (x0 + radX)) / radX;
                dy = (y - (y1 - radY)) / radY;
                inCorner = true;
            } else if (x > x1 - radX && y > y1 - radY) {
                dx = (x - (x1 - radX)) / radX;
                dy = (y - (y1 - radY)) / radY;
                inCorner = true;
            }

            if (!inCorner || (dx * dx + dy * dy) <= 1.0f) {
                SkRasterizer_PlotPixel(dst, w, h, rowBytes, clip, x, y, color);
            }
        }
    }
}

// 4. Draw Circle
void SkRasterizer_DrawCircle(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip,
                             const SkMatrix& ctm, SkScalar cx, SkScalar cy, SkScalar radius, const SkPaint& paint) {
    SkScalar mcx, mcy;
    ctm.mapPoints(&mcx, &mcy, cx, cy);
    SkScalar mrad = radius * ctm.getScaleX();

    int l = SkScalarFloorToInt(mcx - mrad);
    int t = SkScalarFloorToInt(mcy - mrad);
    int r = SkScalarCeilToInt(mcx + mrad);
    int b = SkScalarCeilToInt(mcy + mrad);

    l = SkScalarMax((SkScalar)l, (SkScalar)clip.fLeft);
    t = SkScalarMax((SkScalar)t, (SkScalar)clip.fTop);
    r = SkScalarMin((SkScalar)r, (SkScalar)clip.fRight);
    b = SkScalarMin((SkScalar)b, (SkScalar)clip.fBottom);

    if (l >= r || t >= b) return;

    uint32_t color = paint.getColor();
    SkScalar radSq = mrad * mrad;
    SkScalar innerRadSq = 0;
    if (paint.getStyle() == SkPaint::kStroke_Style) {
        SkScalar strokeW = paint.getStrokeWidth() * ctm.getScaleX();
        SkScalar innerRad = mrad - strokeW;
        if (innerRad > 0) innerRadSq = innerRad * innerRad;
    }

    for (int y = t; y < b; y++) {
        SkScalar dy = (SkScalar)y - mcy;
        SkScalar dy2 = dy * dy;
        for (int x = l; x < r; x++) {
            SkScalar dx = (SkScalar)x - mcx;
            SkScalar distSq = dx * dx + dy2;
            if (distSq <= radSq && distSq >= innerRadSq) {
                SkRasterizer_PlotPixel(dst, w, h, rowBytes, clip, x, y, color);
            }
        }
    }
}

// 5. Draw Line (Bresenham / Stepped line plotting)
void SkRasterizer_DrawLine(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip,
                           const SkMatrix& ctm, SkScalar x0, SkScalar y0, SkScalar x1, SkScalar y1, const SkPaint& paint) {
    SkScalar mx0, my0, mx1, my1;
    ctm.mapPoints(&mx0, &my0, x0, y0);
    ctm.mapPoints(&mx1, &my1, x1, y1);

    int ix0 = SkScalarRoundToInt(mx0);
    int iy0 = SkScalarRoundToInt(my0);
    int ix1 = SkScalarRoundToInt(mx1);
    int iy1 = SkScalarRoundToInt(my1);

    int dx = ix1 - ix0;
    int dy = iy1 - iy0;
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;

    int steps = adx > ady ? adx : ady;
    if (steps == 0) {
        SkRasterizer_PlotPixel(dst, w, h, rowBytes, clip, ix0, iy0, paint.getColor());
        return;
    }

    float xInc = (float)dx / (float)steps;
    float yInc = (float)dy / (float)steps;
    float curX = (float)ix0;
    float curY = (float)iy0;

    int strokeW = SkScalarRoundToInt(paint.getStrokeWidth());
    if (strokeW < 1) strokeW = 1;

    for (int i = 0; i <= steps; i++) {
        int px = (int)curX;
        int py = (int)curY;
        for (int sy = -strokeW / 2; sy <= strokeW / 2; sy++) {
            for (int sx = -strokeW / 2; sx <= strokeW / 2; sx++) {
                SkRasterizer_PlotPixel(dst, w, h, rowBytes, clip, px + sx, py + sy, paint.getColor());
            }
        }
        curX += xInc;
        curY += yInc;
    }
}

// 6. Draw Path (Polygon scanline converter)
void SkRasterizer_DrawPath(uint32_t* dst, int w, int h, size_t rowBytes, const SkIRect& clip,
                           const SkMatrix& ctm, const SkPath& path, const SkPaint& paint) {
    if (path.isEmpty()) return;

    // Linearize path points
    std::vector<SkPath::Point> poly;
    const SkPath::Verb* verbs = path.verbs();
    const SkPath::Point* pts = path.points();
    size_t numVerbs = path.countVerbs();
    size_t ptIdx = 0;

    for (size_t i = 0; i < numVerbs; i++) {
        switch (verbs[i]) {
            case SkPath::kMove_Verb:
                if (ptIdx < path.countPoints()) {
                    SkScalar mx, my;
                    ctm.mapPoints(&mx, &my, pts[ptIdx].fX, pts[ptIdx].fY);
                    poly.push_back({mx, my});
                    ptIdx++;
                }
                break;
            case SkPath::kLine_Verb:
                if (ptIdx < path.countPoints()) {
                    SkScalar mx, my;
                    ctm.mapPoints(&mx, &my, pts[ptIdx].fX, pts[ptIdx].fY);
                    poly.push_back({mx, my});
                    ptIdx++;
                }
                break;
            case SkPath::kQuad_Verb:
                if (ptIdx + 1 < path.countPoints()) {
                    SkPath::Point p0 = poly.empty() ? SkPath::Point{0, 0} : poly.back();
                    SkPath::Point p1 = pts[ptIdx++];
                    SkPath::Point p2 = pts[ptIdx++];
                    // Linearize quadratic bezier curve into 8 segments
                    for (int step = 1; step <= 8; step++) {
                        float t = (float)step / 8.0f;
                        float invT = 1.0f - t;
                        float qx = invT * invT * p0.fX + 2.0f * invT * t * p1.fX + t * t * p2.fX;
                        float qy = invT * invT * p0.fY + 2.0f * invT * t * p1.fY + t * t * p2.fY;
                        SkScalar mx, my;
                        ctm.mapPoints(&mx, &my, qx, qy);
                        poly.push_back({mx, my});
                    }
                }
                break;
            case SkPath::kCubic_Verb:
                if (ptIdx + 2 < path.countPoints()) {
                    SkPath::Point p0 = poly.empty() ? SkPath::Point{0, 0} : poly.back();
                    SkPath::Point p1 = pts[ptIdx++];
                    SkPath::Point p2 = pts[ptIdx++];
                    SkPath::Point p3 = pts[ptIdx++];
                    // Linearize cubic bezier into 12 segments
                    for (int step = 1; step <= 12; step++) {
                        float t = (float)step / 12.0f;
                        float invT = 1.0f - t;
                        float cx = invT * invT * invT * p0.fX + 3.0f * invT * invT * t * p1.fX +
                                   3.0f * invT * t * t * p2.fX + t * t * t * p3.fX;
                        float cy = invT * invT * invT * p0.fY + 3.0f * invT * invT * t * p1.fY +
                                   3.0f * invT * t * t * p2.fY + t * t * t * p3.fY;
                        SkScalar mx, my;
                        ctm.mapPoints(&mx, &my, cx, cy);
                        poly.push_back({mx, my});
                    }
                }
                break;
            case SkPath::kClose_Verb:
                if (!poly.empty()) {
                    poly.push_back(poly.front());
                }
                break;
            default:
                break;
        }
    }

    if (poly.size() < 2) return;

    if (paint.getStyle() == SkPaint::kStroke_Style) {
        for (size_t i = 0; i + 1 < poly.size(); i++) {
            SkRasterizer_DrawLine(dst, w, h, rowBytes, clip, SkMatrix::I(),
                                  poly[i].fX, poly[i].fY, poly[i + 1].fX, poly[i + 1].fY, paint);
        }
        return;
    }

    // Fill polygon via scanline intersection
    SkScalar minY = poly[0].fY, maxY = poly[0].fY;
    for (const auto& p : poly) {
        if (p.fY < minY) minY = p.fY;
        if (p.fY > maxY) maxY = p.fY;
    }

    int startY = SkScalarMax((SkScalar)clip.fTop, (SkScalar)SkScalarFloorToInt(minY));
    int endY   = SkScalarMin((SkScalar)clip.fBottom, (SkScalar)SkScalarCeilToInt(maxY));

    std::vector<int> nodeX;
    uint32_t color = paint.getColor();

    for (int y = startY; y < endY; y++) {
        nodeX.clear();
        size_t j = poly.size() - 1;
        for (size_t i = 0; i < poly.size(); i++) {
            if ((poly[i].fY < (SkScalar)y && poly[j].fY >= (SkScalar)y) ||
                (poly[j].fY < (SkScalar)y && poly[i].fY >= (SkScalar)y)) {
                int x = (int)(poly[i].fX + ((SkScalar)y - poly[i].fY) / (poly[j].fY - poly[i].fY) * (poly[j].fX - poly[i].fX));
                nodeX.push_back(x);
            }
            j = i;
        }

        // Sort scanline intersections
        for (size_t i = 0; i + 1 < nodeX.size(); i++) {
            for (size_t k = i + 1; k < nodeX.size(); k++) {
                if (nodeX[i] > nodeX[k]) {
                    int tmp = nodeX[i];
                    nodeX[i] = nodeX[k];
                    nodeX[k] = tmp;
                }
            }
        }

        // Fill horizontal spans
        for (size_t i = 0; i + 1 < nodeX.size(); i += 2) {
            int spanL = SkScalarMax((SkScalar)clip.fLeft, (SkScalar)nodeX[i]);
            int spanR = SkScalarMin((SkScalar)clip.fRight, (SkScalar)nodeX[i + 1]);
            for (int x = spanL; x < spanR; x++) {
                SkRasterizer_PlotPixel(dst, w, h, rowBytes, clip, x, y, color);
            }
        }
    }
}
