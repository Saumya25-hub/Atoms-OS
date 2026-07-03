#include "audio_forensic.h"

static uint8_t g_forensic_pcm_gen[FORENSIC_SAMPLES_COUNT * 2]; // 16-bit
static uint8_t g_forensic_stream_write[FORENSIC_SAMPLES_COUNT * 2];
static uint8_t g_forensic_stream_read[FORENSIC_SAMPLES_COUNT * 2];
static uint8_t g_forensic_mixer_out[FORENSIC_SAMPLES_COUNT * 2];
static uint8_t g_forensic_dma_out[FORENSIC_SAMPLES_COUNT * 2];

static size_t g_pcm_gen_idx = 0;
static size_t g_stream_write_idx = 0;
static size_t g_stream_read_idx = 0;
static size_t g_mixer_out_idx = 0;
static size_t g_dma_out_idx = 0;

void audio_forensic_init(void) {
    g_pcm_gen_idx = 0;
    g_stream_write_idx = 0;
    g_stream_read_idx = 0;
    g_mixer_out_idx = 0;
    g_dma_out_idx = 0;
    
    for (int i = 0; i < FORENSIC_SAMPLES_COUNT * 2; i++) {
        g_forensic_pcm_gen[i] = 0;
        g_forensic_stream_write[i] = 0;
        g_forensic_stream_read[i] = 0;
        g_forensic_mixer_out[i] = 0;
        g_forensic_dma_out[i] = 0;
    }
}

void audio_forensic_log_pcm_gen(const uint8_t* pcm_data, size_t bytes) {
    if (g_pcm_gen_idx >= FORENSIC_SAMPLES_COUNT * 2) return;
    size_t to_copy = bytes;
    if (g_pcm_gen_idx + to_copy > FORENSIC_SAMPLES_COUNT * 2) {
        to_copy = (FORENSIC_SAMPLES_COUNT * 2) - g_pcm_gen_idx;
    }
    for (size_t i = 0; i < to_copy; i++) g_forensic_pcm_gen[g_pcm_gen_idx++] = pcm_data[i];
}

void audio_forensic_log_stream_write(const uint8_t* pcm_data, size_t bytes) {
    if (g_stream_write_idx >= FORENSIC_SAMPLES_COUNT * 2) return;
    size_t to_copy = bytes;
    if (g_stream_write_idx + to_copy > FORENSIC_SAMPLES_COUNT * 2) {
        to_copy = (FORENSIC_SAMPLES_COUNT * 2) - g_stream_write_idx;
    }
    for (size_t i = 0; i < to_copy; i++) g_forensic_stream_write[g_stream_write_idx++] = pcm_data[i];
}

void audio_forensic_log_stream_read(const uint8_t* pcm_data, size_t bytes) {
    if (g_stream_read_idx >= FORENSIC_SAMPLES_COUNT * 2) return;
    size_t to_copy = bytes;
    if (g_stream_read_idx + to_copy > FORENSIC_SAMPLES_COUNT * 2) {
        to_copy = (FORENSIC_SAMPLES_COUNT * 2) - g_stream_read_idx;
    }
    for (size_t i = 0; i < to_copy; i++) g_forensic_stream_read[g_stream_read_idx++] = pcm_data[i];
}

void audio_forensic_log_mixer_out(const uint8_t* pcm_data, size_t bytes) {
    if (g_mixer_out_idx >= FORENSIC_SAMPLES_COUNT * 2) return;
    size_t to_copy = bytes;
    if (g_mixer_out_idx + to_copy > FORENSIC_SAMPLES_COUNT * 2) {
        to_copy = (FORENSIC_SAMPLES_COUNT * 2) - g_mixer_out_idx;
    }
    for (size_t i = 0; i < to_copy; i++) g_forensic_mixer_out[g_mixer_out_idx++] = pcm_data[i];
}

void audio_forensic_log_dma_out(const uint8_t* pcm_data, size_t bytes) {
    if (g_dma_out_idx >= FORENSIC_SAMPLES_COUNT * 2) return;
    size_t to_copy = bytes;
    if (g_dma_out_idx + to_copy > FORENSIC_SAMPLES_COUNT * 2) {
        to_copy = (FORENSIC_SAMPLES_COUNT * 2) - g_dma_out_idx;
    }
    for (size_t i = 0; i < to_copy; i++) g_forensic_dma_out[g_dma_out_idx++] = pcm_data[i];
}

static void print_dump_block(const char* name, const uint8_t* buffer, size_t count_bytes) {
    extern void display_print(const char* str);
    extern void display_print_dec(uint64_t val);
    extern void display_print_hex(uint64_t val);
    
    display_print("[OSCILLOSCOPE_DUMP_START ");
    display_print(name);
    display_print("]\n");
    
    for (size_t i = 0; i < count_bytes / 2; i++) {
        int16_t sample = (int16_t)(buffer[i * 2] | (buffer[i * 2 + 1] << 8));
        
        display_print_dec(i);
        display_print(": ");
        
        if (sample < 0) {
            display_print("-");
            display_print_dec((uint32_t)(-sample));
        } else {
            display_print_dec((uint32_t)sample);
        }
        
        display_print(" ");
        display_print_hex((uint16_t)sample);
        display_print("\n");
    }
    
    display_print("[OSCILLOSCOPE_DUMP_END]\n");
}

void audio_forensic_dump_oscilloscope(void) {
    print_dump_block("pcm_generator", g_forensic_pcm_gen, g_pcm_gen_idx);
    print_dump_block("stream_write", g_forensic_stream_write, g_stream_write_idx);
    print_dump_block("stream_read", g_forensic_stream_read, g_stream_read_idx);
    print_dump_block("mixer_output", g_forensic_mixer_out, g_mixer_out_idx);
    print_dump_block("dma_output", g_forensic_dma_out, g_dma_out_idx);
}
