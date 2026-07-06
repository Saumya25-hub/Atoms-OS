#ifndef STEP14_TELEMETRY_H
#define STEP14_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

/* STEP 14 TEMPORARY RUNTIME INSTRUMENTATION */

typedef struct {
    // 1. Presentation Path
    uint64_t frame_count;
    uint32_t legacy_swapfull_frames;
    uint32_t partial_copy_frames;
    uint32_t dual_page_frames;
    uint32_t last_dirty_count;
    uint32_t last_effective_count;
    uint64_t total_bytes_copied;
    uint64_t total_rects_copied;
    uint32_t last_frame_time_us;
    uint32_t last_present_time_us;

    // 2. Cursor Pipeline
    uint32_t bvcursor_draw_count;
    uint32_t bspe_set_position_count;
    uint32_t hw_cursor_updates;
    uint32_t sw_cursor_draws;

    // 3. Frame Timeline (in microseconds)
    uint64_t last_irq_timestamp_ms;
    uint32_t irq_to_queue_us;
    uint32_t queue_to_pump_us;
    uint32_t pump_duration_us;
    uint32_t hit_test_duration_us;
    uint32_t compositor_duration_us;
    uint32_t cursor_draw_duration_us;
    uint32_t swap_duration_us;
    uint32_t vram_copy_duration_us;
    uint32_t page_flip_duration_us;

    // 4. Login Screen (in microseconds)
    uint32_t login_total_render_us;
    uint32_t login_bg_restore_us;
    uint32_t login_dynamic_controls_us;
    uint32_t login_eye_icon_us;
    uint32_t login_password_us;
    uint32_t login_button_us;
    uint32_t login_caret_us;

    // 5. Desktop (in microseconds)
    uint32_t desktop_hit_test_us;
    uint32_t desktop_window_render_us;
    uint32_t desktop_cursor_render_us;
    uint32_t desktop_present_us;

    // 6. Damage Tracking
    uint32_t dirty_rects_created;
    uint32_t dirty_rects_reach_bspe;
    uint32_t dirty_rects_discarded;
    uint32_t dirty_rects_merged;
    bool dirty_count_becomes_zero;

    // 7. VRAM Copy Stats
    uint32_t bytes_copied_this_frame;
    uint32_t avg_bytes_per_frame;
    uint32_t max_bytes_per_frame;
    uint32_t min_bytes_per_frame;

    // 8. Queue Analysis
    uint32_t irq_queue_len;
    uint32_t kernel_queue_len;
    uint32_t avg_queue_delay_us;
    uint32_t max_queue_delay_us;
    uint32_t dropped_events;
    uint32_t worst_case_delay_us;

    // 9. Frame Clock
    uint32_t actual_fps;
    uint32_t actual_frame_time_ms;
    uint32_t worst_frame_time_ms;
    uint32_t best_frame_time_ms;
    uint32_t avg_frame_time_ms;
    uint32_t max_mouse_latency_ms;
} Step14_Telemetry;

extern Step14_Telemetry g_step14_telemetry;

// Precise cycle/us timing
uint64_t step14_rdtsc(void);
uint32_t step14_cycles_to_us(uint64_t cycles);

// Hook callbacks
void step14_log_irq(void);
void step14_log_queue_push(int qlen);
void step14_log_pump_start(uint64_t irq_time_ms);
void step14_log_hit_test_done(uint32_t duration_us);
void step14_log_pump_done(uint32_t duration_us);
void step14_log_compositor_start(void);
void step14_log_compositor_done(uint32_t dirty_count, uint32_t duration_us);
void step14_log_cursor_draw(uint32_t duration_us);
void step14_log_swapfull(uint32_t dirty_count, uint32_t duration_us, uint32_t bytes_copied);
void step14_log_login_render(uint32_t total_us, uint32_t bg_us, uint32_t dyn_us, uint32_t eye_us, uint32_t pass_us, uint32_t btn_us);

void step14_telemetry_on_frame(void);

#endif
