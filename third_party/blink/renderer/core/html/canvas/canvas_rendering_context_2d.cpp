/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "canvas_rendering_context_2d.h"
#include "userspace/runtime/c/include/string.h"

namespace blink {

CanvasRenderingContext2D::CanvasRenderingContext2D(uint32_t width, uint32_t height)
    : width_(width > 0 ? width : 300),
      height_(height > 0 ? height : 150),
      fill_style_("#000000"),
      stroke_style_("#000000"),
      line_width_(1.0f),
      has_path_(false),
      current_x_(0.0f),
      current_y_(0.0f) {
    pixels_.resize((size_t)width_ * height_ * 4, 0);
}

CanvasRenderingContext2D::~CanvasRenderingContext2D() {
}

CanvasRenderingContext2D* CanvasRenderingContext2D::Create(uint32_t width, uint32_t height) {
    return new CanvasRenderingContext2D(width, height);
}

void CanvasRenderingContext2D::fillRect(float x, float y, float w, float h) {
    int32_t x0 = (int32_t)x;
    int32_t y0 = (int32_t)y;
    int32_t x1 = x0 + (int32_t)w;
    int32_t y1 = y0 + (int32_t)h;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > (int32_t)width_) x1 = (int32_t)width_;
    if (y1 > (int32_t)height_) y1 = (int32_t)height_;

    uint8_t r = 255, g = 0, b = 0, a = 255;
    if (fill_style_ == "#000000" || fill_style_ == "black") {
        r = 0; g = 0; b = 0;
    } else if (fill_style_ == "#FFFFFF" || fill_style_ == "white") {
        r = 255; g = 255; b = 255;
    } else if (fill_style_ == "#0000FF" || fill_style_ == "blue") {
        r = 0; g = 0; b = 255;
    } else if (fill_style_ == "#00FF00" || fill_style_ == "green") {
        r = 0; g = 255; b = 0;
    }

    for (int32_t cy = y0; cy < y1; cy++) {
        for (int32_t cx = x0; cx < x1; cx++) {
            size_t idx = ((size_t)cy * width_ + cx) * 4;
            if (idx + 3 < pixels_.size()) {
                pixels_[idx + 0] = r;
                pixels_[idx + 1] = g;
                pixels_[idx + 2] = b;
                pixels_[idx + 3] = a;
            }
        }
    }
}

void CanvasRenderingContext2D::strokeRect(float x, float y, float w, float h) {
    fillRect(x, y, w, 1.0f);
    fillRect(x, y + h - 1.0f, w, 1.0f);
    fillRect(x, y, 1.0f, h);
    fillRect(x + w - 1.0f, y, 1.0f, h);
}

void CanvasRenderingContext2D::clearRect(float x, float y, float w, float h) {
    int32_t x0 = (int32_t)x;
    int32_t y0 = (int32_t)y;
    int32_t x1 = x0 + (int32_t)w;
    int32_t y1 = y0 + (int32_t)h;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > (int32_t)width_) x1 = (int32_t)width_;
    if (y1 > (int32_t)height_) y1 = (int32_t)height_;

    for (int32_t cy = y0; cy < y1; cy++) {
        for (int32_t cx = x0; cx < x1; cx++) {
            size_t idx = ((size_t)cy * width_ + cx) * 4;
            if (idx + 3 < pixels_.size()) {
                pixels_[idx + 0] = 0;
                pixels_[idx + 1] = 0;
                pixels_[idx + 2] = 0;
                pixels_[idx + 3] = 0;
            }
        }
    }
}

void CanvasRenderingContext2D::beginPath() {
    has_path_ = true;
    current_x_ = 0.0f;
    current_y_ = 0.0f;
}

void CanvasRenderingContext2D::moveTo(float x, float y) {
    current_x_ = x;
    current_y_ = y;
}

void CanvasRenderingContext2D::lineTo(float x, float y) {
    current_x_ = x;
    current_y_ = y;
}

void CanvasRenderingContext2D::stroke() {
    if (!has_path_) return;
}

void CanvasRenderingContext2D::fill() {
    if (!has_path_) return;
}

void CanvasRenderingContext2D::arc(float x, float y, float radius, float startAngle, float endAngle, bool anticlockwise) {
    (void)x; (void)y; (void)radius; (void)startAngle; (void)endAngle; (void)anticlockwise;
}

ImageData CanvasRenderingContext2D::getImageData(uint32_t sx, uint32_t sy, uint32_t sw, uint32_t sh) {
    ImageData img;
    img.width = sw;
    img.height = sh;
    img.data.resize((size_t)sw * sh * 4, 0);

    for (uint32_t cy = 0; cy < sh; cy++) {
        for (uint32_t cx = 0; cx < sw; cx++) {
            uint32_t src_x = sx + cx;
            uint32_t src_y = sy + cy;
            if (src_x < width_ && src_y < height_) {
                size_t src_idx = ((size_t)src_y * width_ + src_x) * 4;
                size_t dst_idx = ((size_t)cy * sw + cx) * 4;
                if (src_idx + 3 < pixels_.size() && dst_idx + 3 < img.data.size()) {
                    img.data[dst_idx + 0] = pixels_[src_idx + 0];
                    img.data[dst_idx + 1] = pixels_[src_idx + 1];
                    img.data[dst_idx + 2] = pixels_[src_idx + 2];
                    img.data[dst_idx + 3] = pixels_[src_idx + 3];
                }
            }
        }
    }
    return img;
}

void CanvasRenderingContext2D::putImageData(const ImageData& data, uint32_t dx, uint32_t dy) {
    for (uint32_t cy = 0; cy < data.height; cy++) {
        for (uint32_t cx = 0; cx < data.width; cx++) {
            uint32_t dst_x = dx + cx;
            uint32_t dst_y = dy + cy;
            if (dst_x < width_ && dst_y < height_) {
                size_t src_idx = ((size_t)cy * data.width + cx) * 4;
                size_t dst_idx = ((size_t)dst_y * width_ + dst_x) * 4;
                if (src_idx + 3 < data.data.size() && dst_idx + 3 < pixels_.size()) {
                    pixels_[dst_idx + 0] = data.data[src_idx + 0];
                    pixels_[dst_idx + 1] = data.data[src_idx + 1];
                    pixels_[dst_idx + 2] = data.data[src_idx + 2];
                    pixels_[dst_idx + 3] = data.data[src_idx + 3];
                }
            }
        }
    }
}

std::string CanvasRenderingContext2D::toDataURL() const {
    return "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";
}

} // namespace blink
