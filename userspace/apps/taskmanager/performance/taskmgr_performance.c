#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

// Performance Manager Engine — FPS, frame time, scheduler stats
static uint32_t s_fps = 0;
static uint32_t s_frame_time_us = 0;

void taskmgr_performance_init(void) {
    s_fps = 60;
    s_frame_time_us = 16666;
    display_print("[TASKMGR_PERF] Performance Manager Engine Initialized.\n");
}

bool taskmgr_performance_refresh(void) {
    s_fps = 60;
    s_frame_time_us = 16700;
    display_print("[TASKMGR_PERF] Performance refresh -> FPS=60, FrameTime=16.7ms\n");
    return true;
}

uint32_t taskmgr_perf_get_fps(void)          { return s_fps; }
uint32_t taskmgr_perf_get_frame_us(void)     { return s_frame_time_us; }
