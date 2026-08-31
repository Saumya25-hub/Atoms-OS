/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_CANVAS_RENDERING_CONTEXT_2D_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_CANVAS_RENDERING_CONTEXT_2D_H_

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

namespace blink {

struct ImageData {
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> data;
};

class CanvasRenderingContext2D {
public:
    CanvasRenderingContext2D(uint32_t width = 300, uint32_t height = 150);
    ~CanvasRenderingContext2D();

    static CanvasRenderingContext2D* Create(uint32_t width, uint32_t height);

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    const uint8_t* pixels() const { return pixels_.empty() ? nullptr : &pixels_[0]; }

    // Colors & Styles
    void setFillStyle(const std::string& style) { fill_style_ = style; }
    const std::string& fillStyle() const { return fill_style_; }
    void setStrokeStyle(const std::string& style) { stroke_style_ = style; }
    const std::string& strokeStyle() const { return stroke_style_; }
    void setLineWidth(float width) { line_width_ = width; }
    float lineWidth() const { return line_width_; }

    // Rectangles
    void fillRect(float x, float y, float w, float h);
    void strokeRect(float x, float y, float w, float h);
    void clearRect(float x, float y, float w, float h);

    // Paths
    void beginPath();
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void stroke();
    void fill();
    void arc(float x, float y, float radius, float startAngle, float endAngle, bool anticlockwise = false);

    // Pixel Data
    ImageData getImageData(uint32_t sx, uint32_t sy, uint32_t sw, uint32_t sh);
    void putImageData(const ImageData& data, uint32_t dx, uint32_t dy);

    std::string toDataURL() const;

private:
    uint32_t width_;
    uint32_t height_;
    std::vector<uint8_t> pixels_;
    std::string fill_style_;
    std::string stroke_style_;
    float line_width_;
    bool has_path_;
    float current_x_;
    float current_y_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_CANVAS_RENDERING_CONTEXT_2D_H_
