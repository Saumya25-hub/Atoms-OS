/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "media_source.h"

namespace blink {

SourceBuffer::SourceBuffer(const std::string& type)
    : type_(type),
      updating_(false) {
}

SourceBuffer::~SourceBuffer() {
}

void SourceBuffer::appendBuffer(const uint8_t* data, size_t size) {
    if (!data || size == 0) return;
    updating_ = true;
    size_t old_size = buffer_.size();
    buffer_.resize(old_size + size);
    for (size_t i = 0; i < size; i++) {
        buffer_[old_size + i] = data[i];
    }
    updating_ = false;
}

void SourceBuffer::remove(double start, double end) {
    (void)start; (void)end;
    buffer_.clear();
}

void SourceBuffer::abort() {
    updating_ = false;
}

MediaSource::MediaSource()
    : ready_state_(MSE_OPEN),
      duration_(0.0) {
}

MediaSource::~MediaSource() {
    for (size_t i = 0; i < source_buffers_.size(); i++) {
        delete source_buffers_[i];
    }
    source_buffers_.clear();
}

SourceBuffer* MediaSource::addSourceBuffer(const std::string& type) {
    if (!isTypeSupported(type)) return nullptr;
    SourceBuffer* sb = new SourceBuffer(type);
    source_buffers_.push_back(sb);
    return sb;
}

void MediaSource::endOfStream() {
    ready_state_ = MSE_ENDED;
}

bool MediaSource::isTypeSupported(const std::string& type) {
    return (type == "video/mp4" || type == "audio/mp4" ||
            type == "video/webm" || type == "audio/webm" ||
            type == "video/pcm" || type == "audio/pcm");
}

} // namespace blink
