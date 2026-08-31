/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "video_decoder.h"

namespace blink {

VideoFrame::VideoFrame(uint32_t width, uint32_t height, int64_t timestamp_us)
    : width_(width),
      height_(height),
      timestamp_us_(timestamp_us) {
    frame_data_.resize((size_t)width_ * height_ * 4, 0);
}

VideoFrame::~VideoFrame() {
    close();
}

void VideoFrame::close() {
    frame_data_.clear();
    width_ = 0;
    height_ = 0;
}

VideoDecoder::VideoDecoder()
    : state_(CODEC_UNCONFIGURED),
      decode_queue_size_(0),
      last_frame_(nullptr) {
}

VideoDecoder::~VideoDecoder() {
    close();
}

void VideoDecoder::configure(const VideoDecoderConfig& config) {
    config_ = config;
    state_ = CODEC_CONFIGURED;
}

void VideoDecoder::decode(const uint8_t* chunk_data, size_t chunk_size, int64_t timestamp_us) {
    (void)chunk_data; (void)chunk_size;
    if (state_ != CODEC_CONFIGURED) return;

    if (last_frame_) {
        delete last_frame_;
    }
    last_frame_ = new VideoFrame(config_.codedWidth, config_.codedHeight, timestamp_us);
    decode_queue_size_ = 0;
}

void VideoDecoder::flush() {
}

void VideoDecoder::reset() {
    if (last_frame_) {
        delete last_frame_;
        last_frame_ = nullptr;
    }
    decode_queue_size_ = 0;
}

void VideoDecoder::close() {
    reset();
    state_ = CODEC_CLOSED;
}

} // namespace blink
