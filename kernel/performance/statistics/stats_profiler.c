/**
 * @file stats_profiler.c
 * @brief Performance Statistics Aggregator & Top 10 Hotspot Ranker
 */

#include "../include/profiler.h"

#if BOS_ENABLE_PROFILER

static BOS_ProfStats s_global_stats = {0};
static uint32_t     s_rolling_times[BOS_PROFILER_ROLLING_WINDOW];
static uint32_t     s_rolling_dirty[BOS_PROFILER_ROLLING_WINDOW];
static uint32_t     s_rolling_vram[BOS_PROFILER_ROLLING_WINDOW];
static uint32_t     s_rolling_head = 0;
static uint32_t     s_rolling_count = 0;

static BOS_ProfHotspot s_hotspot_registry[BOS_PROFILER_MAX_HOTSPOTS];
static uint32_t        s_hotspot_count = 0;

void bos_prof_hotspot_record(const char* func_name, uint64_t elapsed_cycles) {
    if (!func_name) return;
    
    // Find existing entry
    for (uint32_t i = 0; i < s_hotspot_count; i++) {
        if (s_hotspot_registry[i].func_name == func_name) {
            s_hotspot_registry[i].call_count++;
            s_hotspot_registry[i].total_cycles += elapsed_cycles;
            if (elapsed_cycles < s_hotspot_registry[i].min_cycles) s_hotspot_registry[i].min_cycles = elapsed_cycles;
            if (elapsed_cycles > s_hotspot_registry[i].max_cycles) s_hotspot_registry[i].max_cycles = elapsed_cycles;
            s_hotspot_registry[i].avg_cycles = s_hotspot_registry[i].total_cycles / s_hotspot_registry[i].call_count;
            s_hotspot_registry[i].avg_us = (uint32_t)((s_hotspot_registry[i].avg_cycles * 1000000ULL) / 2000000000ULL);
            return;
        }
    }
    
    // Add new entry if space available
    if (s_hotspot_count < BOS_PROFILER_MAX_HOTSPOTS) {
        s_hotspot_registry[s_hotspot_count].func_name = func_name;
        s_hotspot_registry[s_hotspot_count].call_count = 1;
        s_hotspot_registry[s_hotspot_count].total_cycles = elapsed_cycles;
        s_hotspot_registry[s_hotspot_count].min_cycles = elapsed_cycles;
        s_hotspot_registry[s_hotspot_count].max_cycles = elapsed_cycles;
        s_hotspot_registry[s_hotspot_count].avg_cycles = elapsed_cycles;
        s_hotspot_registry[s_hotspot_count].avg_us = (uint32_t)((elapsed_cycles * 1000000ULL) / 2000000000ULL);
        s_hotspot_count++;
    }
}

static void sort_and_rank_top_hotspots(void) {
    // Sort hotspot registry in descending order of avg_cycles (Insertion Sort)
    for (uint32_t i = 1; i < s_hotspot_count; i++) {
        BOS_ProfHotspot key = s_hotspot_registry[i];
        int32_t j = (int32_t)i - 1;
        while (j >= 0 && s_hotspot_registry[j].avg_cycles < key.avg_cycles) {
            s_hotspot_registry[j + 1] = s_hotspot_registry[j];
            j--;
        }
        s_hotspot_registry[j + 1] = key;
    }
    
    // Copy Top 10 to stats output
    uint32_t rank_limit = (s_hotspot_count < BOS_PROFILER_TOP_RANK) ? s_hotspot_count : BOS_PROFILER_TOP_RANK;
    s_global_stats.hotspot_count = rank_limit;
    for (uint32_t k = 0; k < rank_limit; k++) {
        s_global_stats.top_hotspots[k] = s_hotspot_registry[k];
    }
}

void bos_prof_stats_update(const BOS_FrameMetrics* frame) {
    if (!frame) return;
    
    s_global_stats.total_frames++;
    uint32_t frame_time_us = frame->frame_time_us;
    
    // Rolling Window Update
    s_rolling_times[s_rolling_head] = frame_time_us;
    s_rolling_dirty[s_rolling_head] = frame->dirty_rect_area;
    s_rolling_vram[s_rolling_head]  = (uint32_t)frame->vram_bytes_copied;
    s_rolling_head = (s_rolling_head + 1) % BOS_PROFILER_ROLLING_WINDOW;
    if (s_rolling_count < BOS_PROFILER_ROLLING_WINDOW) s_rolling_count++;
    
    uint64_t rolling_sum = 0;
    uint64_t dirty_sum = 0;
    uint64_t vram_sum = 0;
    for (uint32_t r = 0; r < s_rolling_count; r++) {
        rolling_sum += s_rolling_times[r];
        dirty_sum   += s_rolling_dirty[r];
        vram_sum    += s_rolling_vram[r];
    }
    s_global_stats.avg_frame_time_us = (uint32_t)(rolling_sum / s_rolling_count);
    s_global_stats.avg_dirty_area = (uint32_t)(dirty_sum / s_rolling_count);
    s_global_stats.avg_vram_copy_bytes = (uint32_t)(vram_sum / s_rolling_count);
    s_global_stats.current_fps = (s_global_stats.avg_frame_time_us > 0) ? (1000000U / s_global_stats.avg_frame_time_us) : 0;
    s_global_stats.total_memory_bandwidth_bytes_per_sec = (uint64_t)s_global_stats.avg_vram_copy_bytes * s_global_stats.current_fps;
    
    // Best & Worst Trackers
    if (s_global_stats.total_frames == 1 || frame_time_us > s_global_stats.worst_frame_time_us) {
        s_global_stats.worst_frame_time_us = frame_time_us;
        s_global_stats.worst_frame_id = frame->frame_id;
    }
    if (s_global_stats.total_frames == 1 || frame_time_us < s_global_stats.best_frame_time_us) {
        s_global_stats.best_frame_time_us = frame_time_us;
        s_global_stats.best_frame_id = frame->frame_id;
    }
    
    sort_and_rank_top_hotspots();
}

void bos_profiler_get_stats(BOS_ProfStats* out_stats) {
    if (out_stats) {
        *out_stats = s_global_stats;
    }
}

#endif /* BOS_ENABLE_PROFILER */
