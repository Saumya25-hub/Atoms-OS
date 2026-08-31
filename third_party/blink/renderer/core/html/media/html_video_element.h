/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_VIDEO_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_VIDEO_ELEMENT_H_

#include "html_media_element.h"
#include "userspace/runtime/cpp/include/vector"

namespace blink {

class HTMLVideoElement : public HTMLMediaElement {
public:
    HTMLVideoElement(uint32_t width = 640, uint32_t height = 360);
    ~HTMLVideoElement() override;

    uint32_t videoWidth() const { return video_width_; }
    uint32_t videoHeight() const { return video_height_; }
    uint32_t width() const { return width_; }
    void setWidth(uint32_t w) { width_ = w; }
    uint32_t height() const { return height_; }
    void setHeight(uint32_t h) { height_ = h; }

    bool RenderFrame(uint8_t* out_surface, uint32_t surface_width, uint32_t surface_height);

private:
    uint32_t width_;
    uint32_t height_;
    uint32_t video_width_;
    uint32_t video_height_;
    std::vector<uint8_t> frame_buffer_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_VIDEO_ELEMENT_H_
