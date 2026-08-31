/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "audio_context.h"

namespace blink {

AudioNode::AudioNode()
    : destination_(nullptr) {
}

AudioNode::~AudioNode() {
    disconnect();
}

void AudioNode::connect(AudioNode* destination) {
    destination_ = destination;
}

void AudioNode::disconnect() {
    destination_ = nullptr;
}

GainNode::GainNode(float gain)
    : gain_(gain) {
}

GainNode::~GainNode() {
}

AudioBufferSourceNode::AudioBufferSourceNode()
    : is_playing_(false) {
}

AudioBufferSourceNode::~AudioBufferSourceNode() {
}

void AudioBufferSourceNode::start(double when) {
    (void)when;
    is_playing_ = true;
}

void AudioBufferSourceNode::stop(double when) {
    (void)when;
    is_playing_ = false;
}

AudioContext::AudioContext(uint32_t sample_rate)
    : sample_rate_(sample_rate),
      current_time_(0.0),
      state_(AUDIO_CONTEXT_RUNNING) {
}

AudioContext::~AudioContext() {
    close();
}

GainNode* AudioContext::createGain() {
    GainNode* node = new GainNode();
    nodes_.push_back(node);
    return node;
}

AudioBufferSourceNode* AudioContext::createBufferSource() {
    AudioBufferSourceNode* node = new AudioBufferSourceNode();
    nodes_.push_back(node);
    return node;
}

void AudioContext::resume() {
    state_ = AUDIO_CONTEXT_RUNNING;
}

void AudioContext::suspend() {
    state_ = AUDIO_CONTEXT_SUSPENDED;
}

void AudioContext::close() {
    state_ = AUDIO_CONTEXT_CLOSED;
    for (size_t i = 0; i < nodes_.size(); i++) {
        delete nodes_[i];
    }
    nodes_.clear();
}

} // namespace blink
