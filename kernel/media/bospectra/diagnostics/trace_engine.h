/*
 * BOSPECTRA V3 — Trace Engine Subsystem
 * kernel/media/bospectra/diagnostics/trace_engine.h
 *
 * Persistent multimedia event trace ring buffer.
 */

#ifndef BOSPECTRA_V3_TRACE_ENGINE_H
#define BOSPECTRA_V3_TRACE_ENGINE_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t timestamp_us;
    uint32_t session_id;
    char     subsystem[24];
    char     event[48];
    uint32_t object_id;
    bool     is_valid;
} BOSPECTRA_TraceEvent;

void bospectra_trace_engine_init(void);
void bospectra_trace_engine_shutdown(void);

void bospectra_trace_record(uint32_t session_id, const char* subsystem, const char* event, uint32_t object_id);
void bospectra_trace_dump(void);

#endif /* BOSPECTRA_V3_TRACE_ENGINE_H */
