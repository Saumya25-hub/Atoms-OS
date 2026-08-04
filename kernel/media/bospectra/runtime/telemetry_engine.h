/*
 * BOSPECTRA V3 — Telemetry Engine Subsystem
 * kernel/media/bospectra/runtime/telemetry_engine.h
 */

#ifndef BOSPECTRA_V3_TELEMETRY_ENGINE_H
#define BOSPECTRA_V3_TELEMETRY_ENGINE_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t packets_read;
    uint32_t packets_queued;
    uint32_t packets_dropped;
    uint32_t packets_decoded;
    uint32_t frames_decoded;
    uint32_t frames_presented;
    uint32_t frames_dropped;
    uint32_t frames_skipped;
    int64_t  pts_drift_us;
    int64_t  clock_drift_us;
    uint32_t decode_time_us;
    uint32_t render_time_us;
    uint32_t present_time_us;
} BOSPECTRA_TelemetryData;

void                   bospectra_telemetry_engine_init(void);
void                   bospectra_telemetry_engine_shutdown(void);

void                   bospectra_telemetry_record_frame_decode(uint32_t duration_us);
void                   bospectra_telemetry_record_frame_present(uint32_t duration_us);
BOSPECTRA_TelemetryData bospectra_telemetry_get_data(void);

#endif /* BOSPECTRA_V3_TELEMETRY_ENGINE_H */
