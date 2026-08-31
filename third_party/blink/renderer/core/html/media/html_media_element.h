/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_MEDIA_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_MEDIA_ELEMENT_H_

#include "userspace/runtime/cpp/include/string"
#include <stdint.h>
#include <stdbool.h>

namespace blink {

enum ReadyState {
    HAVE_NOTHING = 0,
    HAVE_METADATA = 1,
    HAVE_CURRENT_DATA = 2,
    HAVE_FUTURE_DATA = 3,
    HAVE_ENOUGH_DATA = 4
};

enum NetworkState {
    NETWORK_EMPTY = 0,
    NETWORK_IDLE = 1,
    NETWORK_LOADING = 2,
    NETWORK_NO_SOURCE = 3
};

class HTMLMediaElement {
public:
    HTMLMediaElement();
    virtual ~HTMLMediaElement();

    const std::string& src() const { return src_; }
    void setSrc(const std::string& src);

    bool paused() const { return paused_; }
    bool ended() const { return ended_; }
    bool muted() const { return muted_; }
    void setMuted(bool mute) { muted_ = mute; }
    float volume() const { return volume_; }
    void setVolume(float vol);

    double currentTime() const { return current_time_; }
    void setCurrentTime(double time);
    double duration() const { return duration_; }

    ReadyState readyState() const { return ready_state_; }
    NetworkState networkState() const { return network_state_; }
    double buffered() const { return buffered_amount_; }

    virtual void play();
    virtual void pause();
    virtual void load();

protected:
    std::string src_;
    bool paused_;
    bool ended_;
    bool muted_;
    float volume_;
    double current_time_;
    double duration_;
    double buffered_amount_;
    ReadyState ready_state_;
    NetworkState network_state_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_MEDIA_ELEMENT_H_
