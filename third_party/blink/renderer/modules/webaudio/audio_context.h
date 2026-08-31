/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_CONTEXT_H_

#include "userspace/runtime/cpp/include/vector"
#include <stdint.h>
#include <stdbool.h>

namespace blink {

enum AudioContextState {
    AUDIO_CONTEXT_SUSPENDED = 0,
    AUDIO_CONTEXT_RUNNING = 1,
    AUDIO_CONTEXT_CLOSED = 2
};

class AudioNode {
public:
    AudioNode();
    virtual ~AudioNode();

    void connect(AudioNode* destination);
    void disconnect();

    AudioNode* destination() const { return destination_; }

protected:
    AudioNode* destination_;
};

class GainNode : public AudioNode {
public:
    GainNode(float gain = 1.0f);
    ~GainNode() override;

    float gain() const { return gain_; }
    void setGain(float gain) { gain_ = gain; }

private:
    float gain_;
};

class AudioBufferSourceNode : public AudioNode {
public:
    AudioBufferSourceNode();
    ~AudioBufferSourceNode() override;

    void start(double when = 0.0);
    void stop(double when = 0.0);

    bool is_playing() const { return is_playing_; }

private:
    bool is_playing_;
};

class AudioContext {
public:
    AudioContext(uint32_t sample_rate = 44100);
    ~AudioContext();

    uint32_t sampleRate() const { return sample_rate_; }
    double currentTime() const { return current_time_; }
    AudioContextState state() const { return state_; }

    GainNode* createGain();
    AudioBufferSourceNode* createBufferSource();
    AudioNode* destination() { return &destination_node_; }

    void resume();
    void suspend();
    void close();

private:
    uint32_t sample_rate_;
    double current_time_;
    AudioContextState state_;
    AudioNode destination_node_;
    std::vector<AudioNode*> nodes_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_CONTEXT_H_
