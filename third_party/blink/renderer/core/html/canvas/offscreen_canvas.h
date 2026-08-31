/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_OFFSCREEN_CANVAS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_OFFSCREEN_CANVAS_H_

#include "canvas_rendering_context_2d.h"
#include "webgl_rendering_context.h"
#include <stdint.h>

namespace blink {

class OffscreenCanvas {
public:
    OffscreenCanvas(uint32_t width = 300, uint32_t height = 150);
    ~OffscreenCanvas();

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

    CanvasRenderingContext2D* getContext2D();
    WebGLRenderingContext* getContextWebGL(gpu::GpuChannelHost* channel = nullptr);

private:
    uint32_t width_;
    uint32_t height_;
    CanvasRenderingContext2D* context_2d_;
    WebGLRenderingContext* context_webgl_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_OFFSCREEN_CANVAS_H_
