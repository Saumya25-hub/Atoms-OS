/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_AUDIO_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_AUDIO_ELEMENT_H_

#include "html_media_element.h"

namespace blink {

class HTMLAudioElement : public HTMLMediaElement {
public:
    HTMLAudioElement();
    ~HTMLAudioElement() override;

    void play() override;
    void pause() override;

    uint32_t audio_stream_id() const { return audio_stream_id_; }
    size_t samples_written() const { return samples_written_; }

private:
    uint32_t audio_stream_id_;
    size_t samples_written_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_AUDIO_ELEMENT_H_
