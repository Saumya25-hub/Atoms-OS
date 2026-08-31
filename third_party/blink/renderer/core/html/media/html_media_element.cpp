/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "html_media_element.h"

namespace blink {

HTMLMediaElement::HTMLMediaElement()
    : paused_(true),
      ended_(false),
      muted_(false),
      volume_(1.0f),
      current_time_(0.0),
      duration_(120.0),
      buffered_amount_(0.0),
      ready_state_(HAVE_NOTHING),
      network_state_(NETWORK_EMPTY) {
}

HTMLMediaElement::~HTMLMediaElement() {
}

void HTMLMediaElement::setSrc(const std::string& src) {
    src_ = src;
    load();
}

void HTMLMediaElement::setVolume(float vol) {
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    volume_ = vol;
}

void HTMLMediaElement::setCurrentTime(double time) {
    if (time < 0.0) time = 0.0;
    if (time > duration_) time = duration_;
    current_time_ = time;
    if (current_time_ >= duration_) {
        ended_ = true;
        paused_ = true;
    }
}

void HTMLMediaElement::load() {
    network_state_ = NETWORK_LOADING;
    ready_state_ = HAVE_METADATA;
    buffered_amount_ = 10.0;
    ready_state_ = HAVE_ENOUGH_DATA;
    network_state_ = NETWORK_IDLE;
}

void HTMLMediaElement::play() {
    if (ended_) {
        current_time_ = 0.0;
        ended_ = false;
    }
    paused_ = false;
}

void HTMLMediaElement::pause() {
    paused_ = true;
}

} // namespace blink
