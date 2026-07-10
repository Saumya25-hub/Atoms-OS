#include "kernel/audio/formats/audio_pcm.h"

// 256-point Sine wave lookup table (scaled to amplitude of 127 for 8-bit signed)
static const int8_t sine_table[256] = {
    0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46,
    49, 51, 54, 57, 60, 63, 65, 68, 71, 73, 76, 78, 81, 83, 85, 88,
    90, 92, 94, 96, 98, 100, 102, 104, 106, 107, 109, 111, 112, 114, 115, 117,
    118, 119, 120, 121, 122, 123, 124, 124, 125, 126, 126, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 126, 126, 125, 124, 124, 123, 122, 121, 120, 119,
    118, 117, 115, 114, 112, 111, 109, 107, 106, 104, 102, 100, 98, 96, 94, 92,
    90, 88, 85, 83, 81, 78, 76, 73, 71, 68, 65, 63, 60, 57, 54, 51,
    49, 46, 43, 40, 37, 34, 31, 28, 25, 22, 19, 16, 12, 9, 6, 3,
    0, -3, -6, -9, -12, -16, -19, -22, -25, -28, -31, -34, -37, -40, -43, -46,
    -49, -51, -54, -57, -60, -63, -65, -68, -71, -73, -76, -78, -81, -83, -85, -88,
    -90, -92, -94, -96, -98, -100, -102, -104, -106, -107, -109, -111, -112, -114, -115, -117,
    -118, -119, -120, -121, -122, -123, -124, -124, -125, -126, -126, -127, -127, -127, -127, -127,
    -127, -127, -127, -127, -127, -127, -126, -126, -125, -124, -124, -123, -122, -121, -120, -119,
    -118, -117, -115, -114, -112, -111, -109, -107, -106, -104, -102, -100, -98, -96, -94, -92,
    -90, -88, -85, -83, -81, -78, -76, -73, -71, -68, -65, -63, -60, -57, -54, -51,
    -49, -46, -43, -40, -37, -34, -31, -28, -25, -22, -19, -16, -12, -9, -6, -3
};

size_t audio_pcm_bytes_per_frame(const AudioPcmFormat* format) {
    if (!format) return 0;
    return (format->bit_depth / 8) * format->channels;
}

bool audio_pcm_format_is_valid(const AudioPcmFormat* format) {
    if (!format) return false;
    if (format->channels == 0 || format->channels > 2) return false;
    if (format->bit_depth != 8 && format->bit_depth != 16) return false;
    if (format->sample_rate == 0) return false;
    return true;
}

bool audio_pcm_packet_is_valid(const AudioPcmPacket* packet) {
    if (!packet) return false;
    if (!packet->pcm_data) return false;
    if (packet->size_bytes == 0 || packet->frame_count == 0) return false;
    if (!audio_pcm_format_is_valid(&packet->format)) return false;
    
    size_t expected_size = packet->frame_count * audio_pcm_bytes_per_frame(&packet->format);
    if (packet->size_bytes != expected_size) return false; // Reject misaligned/corrupt size
    
    return true;
}

void audio_pcm_generate_silence(const AudioPcmFormat* format, uint8_t* buffer, size_t frames) {
    if (!format || !buffer) return;
    
    size_t bytes = frames * audio_pcm_bytes_per_frame(format);
    uint8_t zero_val = (format->bit_depth == 8 && !format->is_signed) ? 128 : 0;
    
    for (size_t i = 0; i < bytes; i++) {
        buffer[i] = zero_val;
    }
}

static uint32_t pcm_isqrt(uint64_t n) {
    if (n == 0) return 0;
    uint64_t x = n;
    uint64_t y = 1;
    while (x > y) {
        x = (x + y) / 2;
        y = n / x;
    }
    return (uint32_t)x;
}

void audio_pcm_generate_sine(const AudioPcmFormat* format, uint32_t freq, uint8_t* buffer, size_t frames, uint32_t* phase_accum) {
    if (!format || !buffer || !phase_accum) return;
    
    uint32_t phase_inc = (freq * 256 * 65536) / format->sample_rate; 
    
    size_t bpf = audio_pcm_bytes_per_frame(format);
    for (size_t i = 0; i < frames; i++) {
        uint8_t table_idx = (*phase_accum >> 16) & 0xFF;
        int8_t val = sine_table[table_idx];
        
        *phase_accum += phase_inc;
        
        for (int c = 0; c < format->channels; c++) {
            if (format->bit_depth == 8) {
                buffer[i * bpf + c] = format->is_signed ? (uint8_t)val : (uint8_t)(val + 128);
            } else if (format->bit_depth == 16) {
                int16_t val16 = val * 256;
                uint16_t uval16 = format->is_signed ? (uint16_t)val16 : (uint16_t)(val16 + 32768);
                buffer[i * bpf + c * 2] = uval16 & 0xFF;
                buffer[i * bpf + c * 2 + 1] = (uval16 >> 8) & 0xFF;
            }
        }
    }
    
    static bool printed_verify = false;
    if (!printed_verify && format->bit_depth == 16 && frames > 0) {
        int32_t min = 32767, max = -32768;
        int64_t sum = 0, sq_sum = 0;
        uint32_t zero_count = 0, non_zero_count = 0;
        
        for (size_t i = 0; i < frames * format->channels; i++) {
            int16_t v = (int16_t)(buffer[i * 2] | (buffer[i * 2 + 1] << 8));
            if (v < min) min = v;
            if (v > max) max = v;
            sum += v;
            sq_sum += (int64_t)v * v;
            if (v == 0) zero_count++;
            else non_zero_count++;
        }
        
        int32_t avg = sum / (frames * format->channels);
        uint32_t rms = pcm_isqrt(sq_sum / (frames * format->channels));
        
        extern void display_print(const char* str);
        extern void display_print_dec(uint64_t val);
        extern void display_print_hex(uint64_t val);
        
        display_print("\n[PCM VERIFY]\n");
        display_print("Frequency: "); display_print_dec(freq); display_print("\n");
        display_print("Generated Samples: "); display_print_dec(frames * format->channels); display_print("\n");
        // Print min/max ignoring negatives for a moment or assuming signedness logic
        // Actually we don't have display_print_int for negatives, so we print hex for negatives or check sign.
        display_print("Min (Hex): "); display_print_hex((uint16_t)min); display_print("\n");
        display_print("Max (Hex): "); display_print_hex((uint16_t)max); display_print("\n");
        display_print("Average (Hex): "); display_print_hex((uint16_t)avg); display_print("\n");
        display_print("RMS: "); display_print_dec(rms); display_print("\n");
        display_print("Zero Count: "); display_print_dec(zero_count); display_print("\n");
        display_print("Non Zero Count: "); display_print_dec(non_zero_count); display_print("\n");
        
        if (max == 0 || non_zero_count == 0) {
            display_print("PASS / FAIL: FAIL\n");
            // Halt
            while(1) { asm volatile("cli; hlt"); }
        } else {
            display_print("PASS / FAIL: PASS\n");
        }
        
        printed_verify = true;
    }
}

void audio_pcm_generate_noise(const AudioPcmFormat* format, uint8_t* buffer, size_t frames, uint32_t* seed) {
    if (!format || !buffer || !seed) return;
    
    size_t bpf = audio_pcm_bytes_per_frame(format);
    for (size_t i = 0; i < frames; i++) {
        for (int c = 0; c < format->channels; c++) {
            *seed = (*seed * 1103515245 + 12345) & 0x7FFFFFFF;
            int8_t val = (int8_t)((*seed >> 16) & 0xFF);
            
            if (format->bit_depth == 8) {
                buffer[i * bpf + c] = format->is_signed ? (uint8_t)val : (uint8_t)(val + 128);
            } else if (format->bit_depth == 16) {
                int16_t val16 = val * 256;
                uint16_t uval16 = format->is_signed ? (uint16_t)val16 : (uint16_t)(val16 + 32768);
                buffer[i * bpf + c * 2] = uval16 & 0xFF;
                buffer[i * bpf + c * 2 + 1] = (uval16 >> 8) & 0xFF;
            }
        }
    }
}

void audio_pcm_generate_square(const AudioPcmFormat* format, uint32_t freq, uint8_t* buffer, size_t frames, uint32_t* phase_accum) {
    if (!format || !buffer || !phase_accum) return;
    
    uint32_t phase_inc = (freq * 256 * 65536) / format->sample_rate; 
    size_t bpf = audio_pcm_bytes_per_frame(format);
    
    for (size_t i = 0; i < frames; i++) {
        uint8_t table_idx = (*phase_accum >> 16) & 0xFF;
        int8_t val = (table_idx < 128) ? 127 : -127;
        
        *phase_accum += phase_inc;
        
        for (int c = 0; c < format->channels; c++) {
            if (format->bit_depth == 8) {
                buffer[i * bpf + c] = format->is_signed ? (uint8_t)val : (uint8_t)(val + 128);
            } else if (format->bit_depth == 16) {
                int16_t val16 = val * 256;
                uint16_t uval16 = format->is_signed ? (uint16_t)val16 : (uint16_t)(val16 + 32768);
                buffer[i * bpf + c * 2] = uval16 & 0xFF;
                buffer[i * bpf + c * 2 + 1] = (uval16 >> 8) & 0xFF;
            }
        }
    }
}

void audio_pcm_generate_saw(const AudioPcmFormat* format, uint32_t freq, uint8_t* buffer, size_t frames, uint32_t* phase_accum) {
    if (!format || !buffer || !phase_accum) return;
    
    uint32_t phase_inc = (freq * 256 * 65536) / format->sample_rate; 
    size_t bpf = audio_pcm_bytes_per_frame(format);
    
    for (size_t i = 0; i < frames; i++) {
        uint8_t table_idx = (*phase_accum >> 16) & 0xFF;
        int8_t val = (int8_t)((table_idx) - 128); // 0-255 -> -128-127
        
        *phase_accum += phase_inc;
        
        for (int c = 0; c < format->channels; c++) {
            if (format->bit_depth == 8) {
                buffer[i * bpf + c] = format->is_signed ? (uint8_t)val : (uint8_t)(val + 128);
            } else if (format->bit_depth == 16) {
                int16_t val16 = val * 256;
                uint16_t uval16 = format->is_signed ? (uint16_t)val16 : (uint16_t)(val16 + 32768);
                buffer[i * bpf + c * 2] = uval16 & 0xFF;
                buffer[i * bpf + c * 2 + 1] = (uval16 >> 8) & 0xFF;
            }
        }
    }
}

void audio_pcm_generate_stereo_sine(const AudioPcmFormat* format, uint32_t freq_l, uint32_t freq_r, uint8_t* buffer, size_t frames, uint32_t* phase_l, uint32_t* phase_r) {
    if (!format || !buffer || !phase_l || !phase_r) return;
    if (format->channels != 2) return; // Must be stereo
    
    uint32_t phase_inc_l = (freq_l * 256 * 65536) / format->sample_rate;
    uint32_t phase_inc_r = (freq_r * 256 * 65536) / format->sample_rate;
    size_t bpf = audio_pcm_bytes_per_frame(format);
    
    for (size_t i = 0; i < frames; i++) {
        uint8_t table_idx_l = (*phase_l >> 16) & 0xFF;
        uint8_t table_idx_r = (*phase_r >> 16) & 0xFF;
        
        int8_t val_l = sine_table[table_idx_l];
        int8_t val_r = sine_table[table_idx_r];
        
        *phase_l += phase_inc_l;
        *phase_r += phase_inc_r;
        
        if (format->bit_depth == 8) {
            buffer[i * bpf + 0] = format->is_signed ? (uint8_t)val_l : (uint8_t)(val_l + 128);
            buffer[i * bpf + 1] = format->is_signed ? (uint8_t)val_r : (uint8_t)(val_r + 128);
        } else if (format->bit_depth == 16) {
            int16_t val16_l = val_l * 256;
            int16_t val16_r = val_r * 256;
            
            uint16_t uval16_l = format->is_signed ? (uint16_t)val16_l : (uint16_t)(val16_l + 32768);
            uint16_t uval16_r = format->is_signed ? (uint16_t)val16_r : (uint16_t)(val16_r + 32768);
            
            buffer[i * bpf + 0] = uval16_l & 0xFF;
            buffer[i * bpf + 1] = (uval16_l >> 8) & 0xFF;
            
            buffer[i * bpf + 2] = uval16_r & 0xFF;
            buffer[i * bpf + 3] = (uval16_r >> 8) & 0xFF;
        }
    }
}

void audio_math_calculate_stats(const AudioPcmFormat* format, const uint8_t* buffer, size_t frames, uint32_t freq, AudioPcmStats* out_stats) {
    if (!format || !buffer || !out_stats) return;
    
    out_stats->frequency = freq;
    out_stats->sample_rate = format->sample_rate;
    out_stats->channels = format->channels;
    out_stats->bits = format->bit_depth;
    out_stats->total_samples = frames * format->channels;
    
    int32_t min = 2147483647;
    int32_t max = -2147483648;
    int64_t sum = 0;
    int64_t sq_sum = 0;
    uint32_t zero_crossings = 0;
    uint32_t checksum = 0;
    
    int32_t prev_val = 0;
    size_t bpf = audio_pcm_bytes_per_frame(format);
    
    for (size_t i = 0; i < out_stats->total_samples; i++) {
        int32_t val = 0;
        
        if (format->bit_depth == 16) {
            int16_t v = (int16_t)(buffer[i * 2] | (buffer[i * 2 + 1] << 8));
            val = format->is_signed ? v : (v - 32768);
            checksum += buffer[i * 2] + buffer[i * 2 + 1];
        } else {
            uint8_t v = buffer[i];
            val = format->is_signed ? (int8_t)v : ((int16_t)v - 128);
            checksum += v;
        }
        
        if (val < min) min = val;
        if (val > max) max = val;
        sum += val;
        sq_sum += (int64_t)val * val;
        
        if (i > 0 && ((prev_val < 0 && val >= 0) || (prev_val >= 0 && val < 0))) {
            zero_crossings++;
        }
        prev_val = val;
    }
    
    out_stats->peak_neg = min;
    out_stats->peak_pos = max;
    out_stats->dc_offset = (int32_t)(sum / (int64_t)out_stats->total_samples);
    out_stats->rms = pcm_isqrt((uint64_t)(sq_sum / out_stats->total_samples));
    out_stats->zero_crossings = zero_crossings;
    out_stats->checksum = checksum;
}
