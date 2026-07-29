/**
 * @file profiler.h
 * @brief ATOMS / BOS OS Performance Profiler Engine (PPE) Master Header
 * @status Production Kernel Subsystem - Phase 1 Architecture
 *
 * @section PURPOSE
 * Provides sub-nanosecond hardware cycle timing (RDTSC), frame pipeline metrics,
 * memory bandwidth profiling, hotspot detection (Top 10 slowest functions),
 * and 300-frame automated diagnostic reporting with 0 release build overhead.
 */

#ifndef BOS_PROFILER_H
#define BOS_PROFILER_H

#include <stdint.h>
#include <stdbool.h>


/* Compile-Time Feature Toggle */
#ifndef BOS_ENABLE_PROFILER
#define BOS_ENABLE_PROFILER 1
#endif

#define BOS_PROFILER_MAX_HOTSPOTS 64
#define BOS_PROFILER_TOP_RANK 10
#define BOS_PROFILER_ROLLING_WINDOW 128
#define BOS_PROFILER_REPORT_INTERVAL 300

/* Section / Metric Identifier Enums */
typedef enum {
    BOS_PROF_STAGE_FRAME_START = 0,
    BOS_PROF_STAGE_INPUT,
    BOS_PROF_STAGE_MOUSE,
    BOS_PROF_STAGE_KEYBOARD,
    BOS_PROF_STAGE_SCHEDULER,
    BOS_PROF_STAGE_TASK_SWITCH,
    BOS_PROF_STAGE_ANIMATIONS,
    BOS_PROF_STAGE_LAYOUT,
    BOS_PROF_STAGE_DESKTOP,
    BOS_PROF_STAGE_LOGIN_SCREEN,
    BOS_PROF_STAGE_WINDOW_MANAGER,
    BOS_PROF_STAGE_COMPOSITOR,
    BOS_PROF_STAGE_DIRTY_RECT_CALC,
    BOS_PROF_STAGE_RENDERER,
    BOS_PROF_STAGE_BLUR_RENDERER,
    BOS_PROF_STAGE_SHADOW_RENDERER,
    BOS_PROF_STAGE_ALPHA_BLEND,
    BOS_PROF_STAGE_TEXT_RENDER,
    BOS_PROF_STAGE_IMAGE_RENDER,
    BOS_PROF_STAGE_CURSOR_RENDER,
    BOS_PROF_STAGE_VRAM_COPY,
    BOS_PROF_STAGE_PRESENT,
    BOS_PROF_STAGE_FRAME_END,
    BOS_PROF_STAGE_COUNT
} BOS_ProfStage;

/* Function Hotspot Entry Structure */
typedef struct {
    const char* func_name;
    uint64_t    call_count;
    uint64_t    total_cycles;
    uint64_t    min_cycles;
    uint64_t    max_cycles;
    uint64_t    avg_cycles;
    uint32_t    avg_us;
} BOS_ProfHotspot;

/* Per-Frame Detailed Timing Snapshot */
typedef struct {
    uint64_t frame_id;
    uint64_t start_tsc;
    uint64_t end_tsc;
    uint64_t stage_tsc[BOS_PROF_STAGE_COUNT];
    
    /* Memory Metrics */
    uint64_t bytes_copied;
    uint64_t vram_bytes_copied;
    uint32_t dirty_rect_area;
    uint32_t total_dirty_rects;
    uint64_t framebuffer_writes;
    uint32_t alloc_count;
    uint32_t free_count;
    uint32_t temp_alloc_count;

    /* Computed Timings */
    uint32_t frame_time_us;
    uint32_t cpu_time_us;
    uint32_t render_time_us;
    uint32_t present_time_us;
    uint32_t vram_copy_time_us;
    uint32_t scheduler_time_us;
} BOS_FrameMetrics;

/* Rolling Aggregate Statistics */
typedef struct {
    uint64_t total_frames;
    uint32_t current_fps;
    uint32_t avg_frame_time_us;
    uint32_t worst_frame_time_us;
    uint32_t best_frame_time_us;
    uint64_t worst_frame_id;
    uint64_t best_frame_id;
    
    uint32_t avg_dirty_area;
    uint32_t avg_vram_copy_bytes;
    uint64_t total_memory_bandwidth_bytes_per_sec;
    
    const char* longest_render_func;
    uint32_t    longest_render_func_us;
    
    BOS_ProfHotspot top_hotspots[BOS_PROFILER_TOP_RANK];
    uint32_t        hotspot_count;
} BOS_ProfStats;

#if BOS_ENABLE_PROFILER

/* High-Precision Hardware Cycle Reading */
static inline uint64_t bos_profiler_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/* Engine Control APIs */
void bos_profiler_init(void);
void bos_profiler_shutdown(void);
void bos_profiler_frame_begin(void);
void bos_profiler_frame_end(void);

/* Stage & Scope Instrumentation APIs */
void bos_profiler_stage_begin(BOS_ProfStage stage);
void bos_profiler_stage_end(BOS_ProfStage stage);

/* Scope Timer Handle for Function Hotspots */
typedef struct {
    const char* func_name;
    uint64_t start_tsc;
} BOS_ProfScopeToken;

BOS_ProfScopeToken bos_profiler_scope_enter(const char* func_name);
void bos_profiler_scope_exit(BOS_ProfScopeToken* token);

/* Metric Recording APIs */
void bos_profiler_record_mem_copy(uint64_t bytes, bool is_vram);
void bos_profiler_record_dirty_rect(int32_t w, int32_t h);
void bos_profiler_record_fb_write(uint32_t count);
void bos_profiler_record_alloc(bool is_temp);
void bos_profiler_record_free(void);

/* Report & Statistics Queries */
void bos_profiler_get_stats(BOS_ProfStats* out_stats);
void bos_profiler_print_report(void);
bool bos_profiler_run_unit_tests(void);

/* Scope Macro Helper */
#define BOS_PROFILE_SCOPE(func_name) \
    BOS_ProfScopeToken __prof_token __attribute__((cleanup(bos_profiler_scope_exit))) = bos_profiler_scope_enter(func_name)

#else

/* Zero-Overhead Release Build Stubs */
static inline void bos_profiler_init(void) {}
static inline void bos_profiler_shutdown(void) {}
static inline void bos_profiler_frame_begin(void) {}
static inline void bos_profiler_frame_end(void) {}
static inline void bos_profiler_stage_begin(BOS_ProfStage stage) { (void)stage; }
static inline void bos_profiler_stage_end(BOS_ProfStage stage) { (void)stage; }
static inline void bos_profiler_record_mem_copy(uint64_t bytes, bool is_vram) { (void)bytes; (void)is_vram; }
static inline void bos_profiler_record_dirty_rect(int32_t w, int32_t h) { (void)w; (void)h; }
static inline void bos_profiler_record_fb_write(uint32_t count) { (void)count; }
static inline void bos_profiler_record_alloc(bool is_temp) { (void)is_temp; }
static inline void bos_profiler_record_free(void) {}
static inline void bos_profiler_print_report(void) {}
static inline bool bos_profiler_run_unit_tests(void) { return true; }

#define BOS_PROFILE_SCOPE(func_name)

#endif /* BOS_ENABLE_PROFILER */

#endif /* BOS_PROFILER_H */
