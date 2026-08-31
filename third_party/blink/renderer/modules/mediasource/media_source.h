/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASOURCE_MEDIA_SOURCE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASOURCE_MEDIA_SOURCE_H_

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>
#include <stddef.h>

namespace blink {

enum ReadyStateMSE {
    MSE_CLOSED = 0,
    MSE_OPEN = 1,
    MSE_ENDED = 2
};

class SourceBuffer {
public:
    SourceBuffer(const std::string& type);
    ~SourceBuffer();

    const std::string& type() const { return type_; }
    bool updating() const { return updating_; }
    size_t buffered_bytes() const { return buffer_.size(); }

    void appendBuffer(const uint8_t* data, size_t size);
    void remove(double start, double end);
    void abort();

private:
    std::string type_;
    bool updating_;
    std::vector<uint8_t> buffer_;
};

class MediaSource {
public:
    MediaSource();
    ~MediaSource();

    ReadyStateMSE readyState() const { return ready_state_; }
    double duration() const { return duration_; }
    void setDuration(double d) { duration_ = d; }

    SourceBuffer* addSourceBuffer(const std::string& type);
    void endOfStream();

    static bool isTypeSupported(const std::string& type);

private:
    ReadyStateMSE ready_state_;
    double duration_;
    std::vector<SourceBuffer*> source_buffers_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASOURCE_MEDIA_SOURCE_H_
