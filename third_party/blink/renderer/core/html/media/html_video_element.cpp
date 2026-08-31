/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "html_video_element.h"

namespace blink {

HTMLVideoElement::HTMLVideoElement(uint32_t width, uint32_t height)
    : width_(width),
      height_(height),
      video_width_(width),
      video_height_(height) {
    size_t size = (size_t)video_width_ * video_height_ * 4;
    frame_buffer_.resize(size, 0);

    for (size_t i = 0; i < size; i += 4) {
        frame_buffer_[i + 0] = 30;  // R
        frame_buffer_[i + 1] = 30;  // G
        frame_buffer_[i + 2] = 46;  // B
        frame_buffer_[i + 3] = 255; // A
    }
}

HTMLVideoElement::~HTMLVideoElement() {
}

bool HTMLVideoElement::RenderFrame(uint8_t* out_surface, uint32_t surface_width, uint32_t surface_height) {
    if (!out_surface || surface_width == 0 || surface_height == 0) return false;

    uint32_t draw_w = (video_width_ < surface_width) ? video_width_ : surface_width;
    uint32_t draw_h = (video_height_ < surface_height) ? video_height_ : surface_height;

    for (uint32_t y = 0; y < draw_h; y++) {
        for (uint32_t x = 0; x < draw_w; x++) {
            size_t src_idx = ((size_t)y * video_width_ + x) * 4;
            size_t dst_idx = ((size_t)y * surface_width + x) * 4;
            out_surface[dst_idx + 0] = frame_buffer_[src_idx + 0];
            out_surface[dst_idx + 1] = frame_buffer_[src_idx + 1];
            out_surface[dst_idx + 2] = frame_buffer_[src_idx + 2];
            out_surface[dst_idx + 3] = frame_buffer_[src_idx + 3];
        }
    }
    return true;
}

} // namespace blink
