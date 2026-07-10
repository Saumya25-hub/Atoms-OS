#ifndef AUDIO_DEBUG_H
#define AUDIO_DEBUG_H

#include <stdint.h>

typedef struct {
    uint32_t active_streams;
    uint32_t allocated_buffers;
    uint64_t allocated_bytes;
    uint32_t peak_streams;
    uint64_t peak_memory;
    uint32_t failed_allocations;
    uint32_t destroyed_streams;
} AudioTelemetry;

void audio_debug_init(void);

void audio_debug_log_alloc(uint32_t bytes);
void audio_debug_log_free(uint32_t bytes);

void audio_debug_log_stream_create(void);
void audio_debug_log_stream_destroy(void);

void audio_debug_log_fail_alloc(void);

void audio_debug_print_stats(void);

void audio_debug_run_selftest(void);
void audio_debug_test_pcm_engine(void);
void audio_debug_test_mixer(void);

#endif // AUDIO_DEBUG_H
