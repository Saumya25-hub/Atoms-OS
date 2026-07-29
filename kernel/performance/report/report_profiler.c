/**
 * @file report_profiler.c
 * @brief Automated 300-Frame Performance Diagnostic Reporter
 */

#include "../include/profiler.h"
#include "kernel/drivers/display/display.h"

#if BOS_ENABLE_PROFILER

extern void bos_prof_stats_update(const BOS_FrameMetrics* frame);

static void print_dec(uint32_t val) {
    display_print_dec(val);
}

static void print_float_1dec(uint32_t val_x10) {
    display_print_dec(val_x10 / 10);
    display_print(".");
    display_print_dec(val_x10 % 10);
}

void bos_profiler_print_report(void) {
    BOS_ProfStats stats;
    bos_profiler_get_stats(&stats);
    
    display_print("\n==============================\n");
    display_print("    BOS PERFORMANCE REPORT    \n");
    display_print("==============================\n");
    
    display_print("Total Frames Instrumented: "); print_dec((uint32_t)stats.total_frames); display_print("\n");
    display_print("Average FPS              : "); print_dec(stats.current_fps); display_print(" FPS\n");
    display_print("Frame Average Time       : "); print_float_1dec(stats.avg_frame_time_us / 100); display_print(" ms\n");
    display_print("Frame Worst Time         : "); print_float_1dec(stats.worst_frame_time_us / 100); display_print(" ms (Frame #"); print_dec((uint32_t)stats.worst_frame_id); display_print(")\n");
    display_print("Frame Best Time          : "); print_float_1dec(stats.best_frame_time_us / 100); display_print(" ms (Frame #"); print_dec((uint32_t)stats.best_frame_id); display_print(")\n");
    display_print("Average Dirty Region Area: "); print_dec(stats.avg_dirty_area); display_print(" px\n");
    display_print("Average VRAM Copy Size   : "); print_dec(stats.avg_vram_copy_bytes); display_print(" bytes\n");
    display_print("Est Memory Bandwidth     : "); print_dec((uint32_t)(stats.total_memory_bandwidth_bytes_per_sec / (1024 * 1024))); display_print(" MB/sec\n");
    
    display_print("\nTop 10 Slowest Functions (Ranked Hotspots):\n");
    display_print("------------------------------------------\n");
    if (stats.hotspot_count == 0) {
        display_print(" (No function hotspot samples recorded)\n");
    } else {
        for (uint32_t i = 0; i < stats.hotspot_count; i++) {
            print_dec(i + 1); display_print(". ");
            display_print(stats.top_hotspots[i].func_name);
            display_print(" -> Avg: ");
            print_float_1dec(stats.top_hotspots[i].avg_us / 100);
            display_print(" ms (Calls: ");
            print_dec((uint32_t)stats.top_hotspots[i].call_count);
            display_print(")\n");
        }
    }
    display_print("==============================\n\n");
}

void bos_prof_report_submodule_check(const BOS_FrameMetrics* frame) {
    if (!frame) return;
    
    bos_prof_stats_update(frame);
    
    if (frame->frame_id > 0 && (frame->frame_id % BOS_PROFILER_REPORT_INTERVAL) == 0) {
        bos_profiler_print_report();
    }
}

#endif /* BOS_ENABLE_PROFILER */
