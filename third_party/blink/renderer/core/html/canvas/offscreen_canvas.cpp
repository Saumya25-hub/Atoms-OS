/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "offscreen_canvas.h"

namespace blink {

OffscreenCanvas::OffscreenCanvas(uint32_t width, uint32_t height)
    : width_(width),
      height_(height),
      context_2d_(nullptr),
      context_webgl_(nullptr) {
}

OffscreenCanvas::~OffscreenCanvas() {
    if (context_2d_) delete context_2d_;
    if (context_webgl_) delete context_webgl_;
}

CanvasRenderingContext2D* OffscreenCanvas::getContext2D() {
    if (!context_2d_) {
        context_2d_ = new CanvasRenderingContext2D(width_, height_);
    }
    return context_2d_;
}

WebGLRenderingContext* OffscreenCanvas::getContextWebGL(gpu::GpuChannelHost* channel) {
    if (!context_webgl_) {
        context_webgl_ = new WebGLRenderingContext(channel);
    }
    return context_webgl_;
}

} // namespace blink
