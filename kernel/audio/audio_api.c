#include "audio_api.h"
#include "audio_core.h"
#include "audio_debug.h"
#include <stddef.h>

void audio_init(void) {
    audio_core_init();
}

void audio_shutdown(void) {
    audio_core_shutdown();
}

uint32_t audio_stream_create(uint32_t process_id) {
    AudioStream* stream = audio_core_register_stream(process_id);
    if (!stream) return 0;
    
    // State transition
    stream->state = AUDIO_STATE_READY;
    return stream->stream_id;
}

bool audio_stream_destroy(uint32_t stream_id) {
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream) return false;
    
    // State transition
    stream->state = AUDIO_STATE_DESTROYED;
    
    return audio_core_destroy_stream(stream_id);
}

bool audio_stream_pause(uint32_t stream_id) {
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream) return false;
    
    if (stream->state == AUDIO_STATE_PLAYING || stream->state == AUDIO_STATE_READY) {
        stream->state = AUDIO_STATE_PAUSED;
        return true;
    }
    return false;
}

bool audio_stream_resume(uint32_t stream_id) {
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream) return false;
    
    if (stream->state == AUDIO_STATE_PAUSED || stream->state == AUDIO_STATE_READY) {
        stream->state = AUDIO_STATE_PLAYING;
        return true;
    }
    return false;
}

bool audio_stream_stop(uint32_t stream_id) {
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream) return false;
    
    if (stream->state == AUDIO_STATE_PLAYING || stream->state == AUDIO_STATE_PAUSED) {
        stream->state = AUDIO_STATE_STOPPED;
        audio_buffer_reset(stream->ring_buffer);
        return true;
    }
    return false;
}

bool audio_set_volume(uint32_t stream_id, uint8_t volume) {
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream) return false;
    
    stream->volume = volume;
    return true;
}

bool audio_stream_set_format(uint32_t stream_id, const AudioPcmFormat* format) {
    if (!format) return false;
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream || stream->state == AUDIO_STATE_DESTROYED) return false;
    
    stream->format = *format;
    return true;
}

size_t audio_stream_write(uint32_t stream_id, const AudioPcmPacket* packet) {
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream || !packet || !packet->pcm_data) return 0;
    if (stream->state == AUDIO_STATE_DESTROYED) return 0;
    if (!audio_pcm_packet_is_valid(packet)) return 0;
    
    if (stream->format.format != packet->format.format ||
        stream->format.sample_rate != packet->format.sample_rate ||
        stream->format.channels != packet->format.channels) {
        return 0;
    }

    size_t written = audio_buffer_write(stream->ring_buffer, packet->pcm_data, packet->size_bytes);
    stream->stats.bytes_written += written;
    if (packet->frame_count > 0 && packet->size_bytes > 0) {
        size_t bpf = packet->size_bytes / packet->frame_count;
        if (bpf > 0) stream->stats.frames_written += (written / bpf);
    }
    
    return written;
}

size_t audio_stream_read(uint32_t stream_id, uint8_t* buffer, size_t size_bytes) {
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream || !buffer || size_bytes == 0) return 0;
    if (stream->state == AUDIO_STATE_DESTROYED) return 0;

    size_t read_bytes = audio_buffer_read(stream->ring_buffer, buffer, size_bytes);
    stream->stats.bytes_read += read_bytes;
    size_t bpf = audio_pcm_bytes_per_frame(&stream->format);
    if (bpf > 0) stream->stats.frames_read += (read_bytes / bpf);
    
    return read_bytes;
}

bool audio_stream_flush(uint32_t stream_id) {
    // Flush effectively drops all current data in the buffer (same as reset)
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream || stream->state == AUDIO_STATE_DESTROYED) return false;
    
    audio_buffer_reset(stream->ring_buffer);
    stream->stats.flush_counter++;
    return true;
}

bool audio_stream_reset(uint32_t stream_id) {
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream || stream->state == AUDIO_STATE_DESTROYED) return false;
    
    audio_buffer_reset(stream->ring_buffer);
    stream->stats.reset_counter++;
    return true;
}

size_t audio_stream_available(uint32_t stream_id) {
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream || stream->state == AUDIO_STATE_DESTROYED) return 0;
    
    return audio_buffer_available(stream->ring_buffer);
}

size_t audio_stream_capacity(uint32_t stream_id) {
    AudioStream* stream = audio_core_get_stream(stream_id);
    if (!stream || stream->state == AUDIO_STATE_DESTROYED) return 0;
    
    if (stream->ring_buffer) {
        return stream->ring_buffer->capacity;
    }
    return 0;
}

void audio_get_stats(void) {
    audio_debug_print_stats();
}
