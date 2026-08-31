/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "html_audio_element.h"
#include "kernel/audio/api/audio_api.h"

namespace blink {

HTMLAudioElement::HTMLAudioElement()
    : audio_stream_id_(0),
      samples_written_(0) {
    audio_stream_id_ = audio_stream_create(1);
    if (audio_stream_id_ > 0) {
        audio_set_volume(audio_stream_id_, 100);
    }
}

HTMLAudioElement::~HTMLAudioElement() {
    if (audio_stream_id_ > 0) {
        audio_stream_stop(audio_stream_id_);
        audio_stream_destroy(audio_stream_id_);
        audio_stream_id_ = 0;
    }
}

void HTMLAudioElement::play() {
    HTMLMediaElement::play();
    if (audio_stream_id_ > 0) {
        audio_stream_resume(audio_stream_id_);
        uint8_t dummy_pcm[128] = { 0 };
        AudioPcmPacket pkt;
        pkt.pcm_data = dummy_pcm;
        pkt.size_bytes = sizeof(dummy_pcm);
        pkt.frame_count = (uint32_t)(sizeof(dummy_pcm) / 2);
        pkt.flags = 0;
        audio_stream_write(audio_stream_id_, &pkt);
        samples_written_ += pkt.frame_count;
    }
}

void HTMLAudioElement::pause() {
    HTMLMediaElement::pause();
    if (audio_stream_id_ > 0) {
        audio_stream_pause(audio_stream_id_);
    }
}

} // namespace blink
