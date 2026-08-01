#ifndef BOS_EXPLORER_PROFILER_H
#define BOS_EXPLORER_PROFILER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t frame_time_us;
    uint32_t layout_time_us;
    uint32_t render_time_us;
    uint32_t dir_read_time_us;
    uint32_t total_memory_bytes;
    uint32_t visible_items_count;
    uint32_t total_items_count;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t repaint_count;
} ExplorerProfiler;

// API
void explorer_profiler_init(ExplorerProfiler* prof);
void explorer_telemetry_dump_json(const ExplorerProfiler* prof);

#endif // BOS_EXPLORER_PROFILER_H
