/**
 * @file mem_profiler.c
 * @brief Memory & Bandwidth Profiler Delegate
 */

#include "../include/profiler.h"

#if BOS_ENABLE_PROFILER

extern BOS_FrameMetrics g_current_frame_metrics;

void bos_profiler_record_mem_copy(uint64_t bytes, bool is_vram) {
    if (is_vram) {
        g_current_frame_metrics.vram_bytes_copied += bytes;
    } else {
        g_current_frame_metrics.bytes_copied += bytes;
    }
}

void bos_profiler_record_dirty_rect(int32_t w, int32_t h) {
    if (w > 0 && h > 0) {
        g_current_frame_metrics.dirty_rect_area += (uint32_t)(w * h);
        g_current_frame_metrics.total_dirty_rects++;
    }
}

void bos_profiler_record_fb_write(uint32_t count) {
    g_current_frame_metrics.framebuffer_writes += count;
}

void bos_profiler_record_alloc(bool is_temp) {
    g_current_frame_metrics.alloc_count++;
    if (is_temp) {
        g_current_frame_metrics.temp_alloc_count++;
    }
}

void bos_profiler_record_free(void) {
    g_current_frame_metrics.free_count++;
}

#endif /* BOS_ENABLE_PROFILER */
