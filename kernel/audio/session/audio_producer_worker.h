#ifndef AUDIO_PRODUCER_WORKER_H
#define AUDIO_PRODUCER_WORKER_H

#include <stdint.h>
#include <stdbool.h>

#define PRODUCER_WATERMARK_HIGH_PCT     80
#define PRODUCER_WATERMARK_TARGET_PCT   70
#define PRODUCER_WATERMARK_LOW_PCT      50
#define PRODUCER_WATERMARK_CRITICAL_PCT 30

typedef struct {
    uint64_t total_chunks_produced;
    uint64_t total_bytes_produced;
    uint64_t total_read_time_ticks;
    uint32_t min_occupancy_pct;
    uint32_t avg_occupancy_pct;
    uint32_t max_occupancy_pct;
    uint32_t current_occupancy_pct;
    bool is_refilling;
} AudioProducerTelemetry;

void audio_producer_worker_init(void);
void audio_producer_worker_run(void);
void audio_producer_worker_get_telemetry(AudioProducerTelemetry* out_stats);

#endif // AUDIO_PRODUCER_WORKER_H
