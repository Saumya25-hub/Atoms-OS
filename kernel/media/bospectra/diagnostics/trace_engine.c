/*
 * BOSPECTRA V3 — Trace Engine Implementation
 * kernel/media/bospectra/diagnostics/trace_engine.c
 */

#include "trace_engine.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_TRACE_RING_CAPACITY 64

extern void display_print(const char* str);

static BOSPECTRA_TraceEvent g_trace_ring[BOSPECTRA_TRACE_RING_CAPACITY];
static uint32_t             g_trace_head = 0;
static uint32_t             g_trace_counter = 0;
static bool                 g_trace_engine_initialized = false;

void bospectra_trace_engine_init(void) {
    memset(g_trace_ring, 0, sizeof(g_trace_ring));
    g_trace_head = 0;
    g_trace_counter = 0;
    g_trace_engine_initialized = true;
    bospectra_log("TRACE_ENGINE", "BOSPECTRA V3 Persistent Trace Engine Initialized.");
}

void bospectra_trace_engine_shutdown(void) {
    g_trace_engine_initialized = false;
}

void bospectra_trace_record(uint32_t session_id, const char* subsystem, const char* event, uint32_t object_id) {
    if (!g_trace_engine_initialized) return;

    BOSPECTRA_TraceEvent* slot = &g_trace_ring[g_trace_head];
    slot->timestamp_us = g_trace_counter * 1000U;
    slot->session_id = session_id;
    if (subsystem) strncpy(slot->subsystem, subsystem, sizeof(slot->subsystem) - 1);
    if (event) strncpy(slot->event, event, sizeof(slot->event) - 1);
    slot->object_id = object_id;
    slot->is_valid = true;

    g_trace_head = (g_trace_head + 1) % BOSPECTRA_TRACE_RING_CAPACITY;
    g_trace_counter++;
}

void bospectra_trace_dump(void) {
    if (!g_trace_engine_initialized) return;

    display_print("\n============= MULTIMEDIA EVENT TRACE LOG =============\n");
    for (size_t i = 0; i < BOSPECTRA_TRACE_RING_CAPACITY; i++) {
        if (g_trace_ring[i].is_valid) {
            display_print("[");
            bospectra_trace_u32("TS_US", (uint32_t)g_trace_ring[i].timestamp_us);
            display_print("] Sess:");
            bospectra_trace_u32("ID", g_trace_ring[i].session_id);
            display_print(" [");
            display_print(g_trace_ring[i].subsystem);
            display_print("] ");
            display_print(g_trace_ring[i].event);
            display_print("\n");
        }
    }
    display_print("======================================================\n");
}
