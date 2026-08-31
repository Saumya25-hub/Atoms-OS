/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBCODECS_VIDEO_DECODER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBCODECS_VIDEO_DECODER_H_

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>
#include <stddef.h>

namespace blink {

enum CodecState {
    CODEC_UNCONFIGURED = 0,
    CODEC_CONFIGURED = 1,
    CODEC_CLOSED = 2
};

struct VideoDecoderConfig {
    std::string codec;
    uint32_t codedWidth;
    uint32_t codedHeight;
};

class VideoFrame {
public:
    VideoFrame(uint32_t width, uint32_t height, int64_t timestamp_us);
    ~VideoFrame();

    uint32_t codedWidth() const { return width_; }
    uint32_t codedHeight() const { return height_; }
    int64_t timestamp() const { return timestamp_us_; }

    void close();

private:
    uint32_t width_;
    uint32_t height_;
    int64_t timestamp_us_;
    std::vector<uint8_t> frame_data_;
};

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    CodecState state() const { return state_; }
    uint32_t decodeQueueSize() const { return decode_queue_size_; }

    void configure(const VideoDecoderConfig& config);
    void decode(const uint8_t* chunk_data, size_t chunk_size, int64_t timestamp_us);
    void flush();
    void reset();
    void close();

    VideoFrame* GetLastFrame() const { return last_frame_; }

private:
    CodecState state_;
    VideoDecoderConfig config_;
    uint32_t decode_queue_size_;
    VideoFrame* last_frame_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBCODECS_VIDEO_DECODER_H_
