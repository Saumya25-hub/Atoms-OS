#include "kernel/audio/mixer/audio_mixer.h"
#include "kernel/audio/api/audio_api.h"
#include "kernel/audio/core/audio_core.h"
#include "kernel/audio/mixer/audio_mix_math.h"
#include "kernel/audio/volume/audio_volume.h"
#include "kernel/audio/mixer/audio_resampler.h"
#include "kernel/audio/mixer/audio_channel.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/drivers/display/display.h"

#define MAX_MIX_STREAMS 32
#define MIX_BUFFER_FRAMES 1024 // e.g., 4096 bytes for 16-bit stereo

static uint32_t g_mixer_streams[MAX_MIX_STREAMS];
static uint32_t g_mixer_stream_count = 0;
static audio_resampler_t g_stream_resamplers[MAX_MIX_STREAMS];
static bool g_resampler_inited[MAX_MIX_STREAMS];

static uint64_t g_frames_mixed = 0;
static uint64_t g_clipped_samples = 0;
static int32_t g_peak_amplitude = 0;
static uint32_t g_last_mixed_streams = 0;

static uint64_t g_mixer_calls = 0;
static uint64_t g_mixer_req_bytes = 0;
static uint64_t g_mixer_ret_bytes = 0;
static uint64_t g_mixer_silence_bytes = 0;

uint64_t g_tl_stream_read_us = 0;

void audio_mixer_init(void) {
    g_mixer_stream_count = 0;
    g_frames_mixed = 0;
    g_clipped_samples = 0;
    g_peak_amplitude = 0;
    g_last_mixed_streams = 0;
    g_mixer_calls = 0;
    g_mixer_req_bytes = 0;
    g_mixer_ret_bytes = 0;
    g_mixer_silence_bytes = 0;
    for (uint32_t i = 0; i < MAX_MIX_STREAMS; i++) {
        g_resampler_inited[i] = false;
    }
    display_print("[AUDIO] Universal Software Mixer Ready (Resampler + Multi-Rate Enabled)\n");
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
    
    uint32_t slot = g_mixer_stream_count++;
    g_mixer_streams[slot] = stream_id;
    g_resampler_inited[slot] = false;
    return true;
}

bool audio_mixer_remove_stream(uint32_t stream_id) {
    for (uint32_t i = 0; i < g_mixer_stream_count; i++) {
        if (g_mixer_streams[i] == stream_id) {
            for (uint32_t j = i; j < g_mixer_stream_count - 1; j++) {
                g_mixer_streams[j] = g_mixer_streams[j+1];
                g_stream_resamplers[j] = g_stream_resamplers[j+1];
                g_resampler_inited[j] = g_resampler_inited[j+1];
            }
            g_mixer_stream_count--;
            return true;
        }
    }
    return false;
}

size_t audio_mixer_process(uint8_t* output_buffer, size_t max_bytes, const AudioPcmFormat* target_format) {
    if (!output_buffer || max_bytes == 0 || !target_format) return 0;
    
    g_mixer_calls++;
    g_mixer_req_bytes += max_bytes;
    
    size_t bpf = audio_pcm_bytes_per_frame(target_format);
    if (bpf == 0) return 0;
    
    size_t frames_to_mix = max_bytes / bpf;
    if (frames_to_mix > MIX_BUFFER_FRAMES) frames_to_mix = MIX_BUFFER_FRAMES;
    
    size_t total_samples = frames_to_mix * target_format->channels;
    size_t mix_bytes = frames_to_mix * bpf;
    
    static int32_t accum_buffer[MIX_BUFFER_FRAMES * 2]; // Max Stereo
    static int16_t temp_in_pcm[MIX_BUFFER_FRAMES * 4];
    static int16_t temp_ch_pcm[MIX_BUFFER_FRAMES * 4];
    static int16_t temp_resample_pcm[MIX_BUFFER_FRAMES * 2];
    
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
        
        if (stream->state == AUDIO_STATE_PAUSED || stream->state == AUDIO_STATE_STOPPED) {
            continue;
        }
        
        uint32_t src_rate = (stream->format.sample_rate > 0) ? stream->format.sample_rate : target_format->sample_rate;
        uint8_t src_channels = (stream->format.channels > 0) ? stream->format.channels : target_format->channels;
        size_t src_bpf = src_channels * (stream->format.bit_depth / 8);
        if (src_bpf == 0) src_bpf = 4;
        
        /* Direct fast path: identical rate and channels */
        if (src_rate == target_format->sample_rate && src_channels == target_format->channels) {
            size_t avail = audio_stream_available(stream_id);
            if (avail >= mix_bytes) {
                size_t bytes_read = audio_stream_read(stream_id, (uint8_t*)temp_in_pcm, mix_bytes);
                if (bytes_read > 0) {
                    size_t samples_read = bytes_read / (stream->format.bit_depth / 8);
                    if (target_format->format == PCM_FORMAT_S16_LE) {
                        audio_volume_apply_16(temp_in_pcm, samples_read, stream->volume, master_vol);
                        audio_math_mix_16(accum_buffer, temp_in_pcm, samples_read);
                    }
                    active_mixed++;
                }
            } else {
                stream->stats.underrun_counter++;
            }
        } else {
            /* Universal Conversion Path: Arbitrary rate and channel conversion */
            if (!g_resampler_inited[i] ||
                g_stream_resamplers[i].in_rate != src_rate ||
                g_stream_resamplers[i].out_rate != target_format->sample_rate) {
                audio_resampler_init(&g_stream_resamplers[i], src_rate, target_format->sample_rate, target_format->channels);
                g_resampler_inited[i] = true;
            }
            
            /* Estimate required input frames */
            uint64_t req_in_f = (((uint64_t)frames_to_mix * src_rate) / target_format->sample_rate) + 4;
            if (req_in_f > (sizeof(temp_in_pcm) / (src_channels * 2))) {
                req_in_f = sizeof(temp_in_pcm) / (src_channels * 2);
            }
            size_t req_in_bytes = (size_t)req_in_f * src_bpf;
            size_t avail = audio_stream_available(stream_id);
            
            if (avail >= req_in_bytes) {
                size_t bytes_read = audio_stream_read(stream_id, (uint8_t*)temp_in_pcm, req_in_bytes);
                size_t in_f = bytes_read / src_bpf;
                if (in_f > 0) {
                    /* Channel layout conversion */
                    audio_channel_convert_16(temp_in_pcm, src_channels, temp_ch_pcm, target_format->channels, in_f);
                    
                    /* Sample rate conversion */
                    size_t in_consumed = 0;
                    size_t out_f = audio_resample_linear_16(&g_stream_resamplers[i],
                                                           temp_ch_pcm, in_f,
                                                           temp_resample_pcm, frames_to_mix,
                                                           &in_consumed);
                    
                    if (out_f > 0) {
                        size_t samples = out_f * target_format->channels;
                        audio_volume_apply_16(temp_resample_pcm, samples, stream->volume, master_vol);
                        audio_math_mix_16(accum_buffer, temp_resample_pcm, samples);
                        active_mixed++;
                    }
                }
            } else {
                stream->stats.underrun_counter++;
            }
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
    
    g_mixer_ret_bytes += mix_bytes;
    if (active_mixed == 0) {
        g_mixer_silence_bytes += mix_bytes;
    }
    
    return mix_bytes;
}

void audio_mixer_get_stats(uint32_t* mixed_streams, uint64_t* frames_mixed, uint64_t* clipped_samples, int32_t* peak_amplitude) {
    if (mixed_streams) *mixed_streams = g_last_mixed_streams;
    if (frames_mixed) *frames_mixed = g_frames_mixed;
    if (clipped_samples) *clipped_samples = g_clipped_samples;
    if (peak_amplitude) *peak_amplitude = g_peak_amplitude;
}

void audio_mixer_get_diag_counters(uint64_t* calls, uint64_t* req, uint64_t* ret, uint64_t* silence) {
    if (calls) *calls = g_mixer_calls;
    if (req) *req = g_mixer_req_bytes;
    if (ret) *ret = g_mixer_ret_bytes;
    if (silence) *silence = g_mixer_silence_bytes;
}
