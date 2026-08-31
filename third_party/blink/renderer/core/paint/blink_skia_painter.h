/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BLINK_SKIA_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BLINK_SKIA_PAINTER_H_

namespace Skia {
class SkCanvas;
}

namespace blink {

class LayoutObject;

class BlinkSkiaPainter {
public:
    static void paint(LayoutObject* root_layout, void* sk_canvas);

private:
    static void paintObject(LayoutObject* obj, void* sk_canvas);
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BLINK_SKIA_PAINTER_H_
