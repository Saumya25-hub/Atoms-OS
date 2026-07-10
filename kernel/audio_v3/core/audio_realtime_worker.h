#ifndef AUDIO_REALTIME_WORKER_H
#define AUDIO_REALTIME_WORKER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Telemetry for the High-Priority Realtime Audio Worker.
 *
 * This worker executes on the REALTIME BOUNDARY (inside IRQ 0 / 1000 Hz timer tick)
 * and strictly monitors hardware CIV/LVI registers and refills memory descriptors via
 * audio_mixer_process() without EVER touching filesystem, disk, malloc/free, or blocking locks.
 */
typedef struct {
    uint64_t total_pumps;
    uint64_t refill_cycles;
    uint32_t max_pump_duration_us;
    uint32_t timing_violations;
    uint32_t last_pump_duration_us;
    bool is_active;
} AudioRealtimeTelemetry;

void audio_realtime_worker_init(void);
void audio_realtime_worker_start(void);
void audio_realtime_worker_stop(void);
void audio_realtime_worker_pump(void);
void audio_realtime_worker_get_telemetry(AudioRealtimeTelemetry* out_stats);

#endif // AUDIO_REALTIME_WORKER_H
