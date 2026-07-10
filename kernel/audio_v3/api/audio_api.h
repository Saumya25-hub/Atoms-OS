#ifndef AUDIO_API_H
#define AUDIO_API_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/audio/formats/audio_pcm.h"

void audio_init(void);
void audio_shutdown(void);

uint32_t audio_stream_create(uint32_t process_id);
bool audio_stream_destroy(uint32_t stream_id);

bool audio_stream_pause(uint32_t stream_id);
bool audio_stream_resume(uint32_t stream_id);
bool audio_stream_stop(uint32_t stream_id);

bool audio_stream_set_format(uint32_t stream_id, const AudioPcmFormat* format);

size_t audio_stream_write(uint32_t stream_id, const AudioPcmPacket* packet);
size_t audio_stream_read(uint32_t stream_id, uint8_t* buffer, size_t size_bytes);
bool audio_stream_flush(uint32_t stream_id);
bool audio_stream_reset(uint32_t stream_id);

size_t audio_stream_available(uint32_t stream_id);
size_t audio_stream_capacity(uint32_t stream_id);

bool audio_set_volume(uint32_t stream_id, uint8_t volume);

void audio_get_stats(void);

#endif // AUDIO_API_H
