/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_IMAGE_BITMAP_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_IMAGE_BITMAP_H_

#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>
#include <stddef.h>

namespace blink {

class ImageBitmap {
public:
    ImageBitmap(uint32_t width, uint32_t height, const uint8_t* pixels = nullptr);
    ~ImageBitmap();

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    const uint8_t* data() const { return data_.empty() ? nullptr : &data_[0]; }
    size_t size_bytes() const { return data_.size(); }

    void close();
    bool is_closed() const { return is_closed_; }

private:
    uint32_t width_;
    uint32_t height_;
    std::vector<uint8_t> data_;
    bool is_closed_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_IMAGE_BITMAP_H_
