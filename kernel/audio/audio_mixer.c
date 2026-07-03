#include "audio_mixer.h"
#include "audio_api.h"
#include "audio_core.h"
#include "audio_mix_math.h"
#include "audio_volume.h"
#include "../core/memory/heap/include/heap.h"

#define MAX_MIX_STREAMS 32
#define MIX_BUFFER_FRAMES 1024 // e.g., 4096 bytes for 16-bit stereo

static uint32_t g_mixer_streams[MAX_MIX_STREAMS];
static uint32_t g_mixer_stream_count = 0;

static uint64_t g_frames_mixed = 0;
static uint64_t g_clipped_samples = 0;
static int32_t g_peak_amplitude = 0;
static uint32_t g_last_mixed_streams = 0;

void audio_mixer_init(void) {
    g_mixer_stream_count = 0;
    g_frames_mixed = 0;
    g_clipped_samples = 0;
    g_peak_amplitude = 0;
    g_last_mixed_streams = 0;
}

void audio_mixer_shutdown(void) {
    g_mixer_stream_count = 0;
}

void audio_mixer_reset(void) {
    g_mixer_stream_count = 0;
}

bool audio_mixer_add_stream(uint32_t stream_id) {
    // Clean up dead streams first to prevent artificial capping
    for (uint32_t i = 0; i < g_mixer_stream_count; ) {
        AudioStream* stream = audio_core_get_stream(g_mixer_streams[i]);
        if (!stream || stream->state == AUDIO_STATE_DESTROYED) {
            audio_mixer_remove_stream(g_mixer_streams[i]);
        } else {
            i++;
        }
    }

    if (g_mixer_stream_count >= MAX_MIX_STREAMS) return false;
    
    // Ensure not already added
    for (uint32_t i = 0; i < g_mixer_stream_count; i++) {
        if (g_mixer_streams[i] == stream_id) return true;
    }
    
    g_mixer_streams[g_mixer_stream_count++] = stream_id;
    return true;
}

bool audio_mixer_remove_stream(uint32_t stream_id) {
    for (uint32_t i = 0; i < g_mixer_stream_count; i++) {
        if (g_mixer_streams[i] == stream_id) {
            for (uint32_t j = i; j < g_mixer_stream_count - 1; j++) {
                g_mixer_streams[j] = g_mixer_streams[j+1];
            }
            g_mixer_stream_count--;
            return true;
        }
    }
    return false;
}

size_t audio_mixer_process(uint8_t* output_buffer, size_t max_bytes, const AudioPcmFormat* target_format) {
    if (!output_buffer || max_bytes == 0 || !target_format) return 0;
    
    size_t bpf = audio_pcm_bytes_per_frame(target_format);
    if (bpf == 0) return 0;
    
    size_t frames_to_mix = max_bytes / bpf;
    if (frames_to_mix > MIX_BUFFER_FRAMES) frames_to_mix = MIX_BUFFER_FRAMES;
    
    size_t total_samples = frames_to_mix * target_format->channels;
    size_t mix_bytes = frames_to_mix * bpf;
    
    static int32_t accum_buffer[MIX_BUFFER_FRAMES * 2]; // Max Stereo
    static uint8_t temp_buffer[MIX_BUFFER_FRAMES * 4];  // Max 16-bit Stereo
    
    // Clear accumulator
    for (size_t i = 0; i < total_samples; i++) {
        accum_buffer[i] = 0;
    }
    
    uint8_t master_vol = audio_volume_get_master();
    uint32_t active_mixed = 0;
    
    for (uint32_t i = 0; i < g_mixer_stream_count; i++) {
        uint32_t stream_id = g_mixer_streams[i];
        AudioStream* stream = audio_core_get_stream(stream_id);
        
        if (!stream || stream->state == AUDIO_STATE_DESTROYED) {
            audio_mixer_remove_stream(stream_id);
            i--;
            continue;
        }
        
        if (stream->format.format != target_format->format ||
            stream->format.channels != target_format->channels) {
            continue;
        }
        
        // Read data from stream ring buffer ONLY if we have enough for a full mix block
        // Reading partial blocks causes time stretching and severe distortion
        size_t avail = audio_stream_available(stream_id);
        if (avail >= mix_bytes) {
            size_t bytes_read = audio_stream_read(stream_id, temp_buffer, mix_bytes);
            
            if (bytes_read > 0) {
                size_t samples_read = bytes_read / (stream->format.bit_depth / 8);
                
                if (target_format->format == PCM_FORMAT_S16_LE) {
                    audio_volume_apply_16((int16_t*)temp_buffer, samples_read, stream->volume, master_vol);
                    audio_math_mix_16(accum_buffer, (int16_t*)temp_buffer, samples_read);
                }
                active_mixed++;
            }
        } else {
            stream->stats.underrun_counter++;
        }
    }
    
    // Normalize and Clamp to Output Buffer
    uint32_t clips = 0;
    if (target_format->format == PCM_FORMAT_S16_LE) {
        audio_math_normalize_16((int16_t*)output_buffer, accum_buffer, total_samples, &clips, &g_peak_amplitude);
    } else {
        for (size_t i = 0; i < mix_bytes; i++) output_buffer[i] = 0;
    }
    
    g_clipped_samples += clips;
    g_frames_mixed += frames_to_mix;
    g_last_mixed_streams = active_mixed;
    
    return mix_bytes;
}

void audio_mixer_get_stats(uint32_t* mixed_streams, uint64_t* frames_mixed, uint64_t* clipped_samples, int32_t* peak_amplitude) {
    if (mixed_streams) *mixed_streams = g_last_mixed_streams;
    if (frames_mixed) *frames_mixed = g_frames_mixed;
    if (clipped_samples) *clipped_samples = g_clipped_samples;
    if (peak_amplitude) *peak_amplitude = g_peak_amplitude;
}
