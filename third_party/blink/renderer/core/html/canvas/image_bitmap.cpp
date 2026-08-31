/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "image_bitmap.h"

namespace blink {

ImageBitmap::ImageBitmap(uint32_t width, uint32_t height, const uint8_t* pixels)
    : width_(width),
      height_(height),
      is_closed_(false) {
    size_t size = (size_t)width_ * height_ * 4;
    data_.resize(size, 0);
    if (pixels) {
        for (size_t i = 0; i < size; i++) {
            data_[i] = pixels[i];
        }
    }
}

ImageBitmap::~ImageBitmap() {
    close();
}

void ImageBitmap::close() {
    data_.clear();
    width_ = 0;
    height_ = 0;
    is_closed_ = true;
}

} // namespace blink
