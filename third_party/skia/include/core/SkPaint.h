/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkPaint_DEFINED
#define SkPaint_DEFINED

#include "SkColor.h"
#include "SkBlendMode.h"

class SkPaint {
public:
    enum Style {
        kFill_Style,
        kStroke_Style,
        kStrokeAndFill_Style,
    };

    enum Cap {
        kButt_Cap,
        kRound_Cap,
        kSquare_Cap,
    };

    enum Join {
        kMiter_Join,
        kRound_Join,
        kBevel_Join,
    };

    SkPaint();
    SkPaint(const SkPaint&);
    ~SkPaint();
    SkPaint& operator=(const SkPaint&);

    void reset();

    Style getStyle() const { return fStyle; }
    void setStyle(Style style) { fStyle = style; }

    SkColor getColor() const { return fColor; }
    void setColor(SkColor color) { fColor = color; }

    SkAlpha getAlpha() const { return SkColorGetA(fColor); }
    void setAlpha(SkAlpha a) { fColor = SkColorSetARGB(a, SkColorGetR(fColor), SkColorGetG(fColor), SkColorGetB(fColor)); }

    SkScalar getStrokeWidth() const { return fWidth; }
    void setStrokeWidth(SkScalar width) { fWidth = width; }

    bool isAntiAlias() const { return fAntiAlias; }
    void setAntiAlias(bool aa) { fAntiAlias = aa; }

    SkBlendMode getBlendMode() const { return fBlendMode; }
    void setBlendMode(SkBlendMode mode) { fBlendMode = mode; }

    Cap getStrokeCap() const { return fCap; }
    void setStrokeCap(Cap cap) { fCap = cap; }

    Join getStrokeJoin() const { return fJoin; }
    void setStrokeJoin(Join join) { fJoin = join; }

private:
    SkColor     fColor;
    SkScalar    fWidth;
    Style       fStyle;
    Cap         fCap;
    Join        fJoin;
    SkBlendMode fBlendMode;
    bool        fAntiAlias;
};

#endif // SkPaint_DEFINED
