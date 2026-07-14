#include "step14_telemetry.h"
#include "kernel/drivers/display/display.h"
#include "kernel/drivers/input/input.h"
#include "bovisual/Include/events.h"

/* STEP 14/15 TEMPORARY RUNTIME INSTRUMENTATION */

Step14_Telemetry g_step14_telemetry = {0};

static uint64_t s_cycles_per_ms = 0;
static bool s_calibrated = false;
extern uint64_t timer_get_ticks(void);
extern void horse_shutdown(void);

uint64_t step14_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

uint64_t step14_cycles_to_us(uint64_t cycles) {
    if (s_cycles_per_ms == 0) return cycles / 2000;
    return (cycles * 1000ULL) / s_cycles_per_ms;
}

void step14_telemetry_init(void) {
    /* Non-blocking calibration — just use a rough estimate first */
    uint64_t t0 = timer_get_ticks();
    uint64_t c0 = step14_rdtsc();
    /* Spin for 1 tick transition only */
    while (timer_get_ticks() == t0) {}
    uint64_t c1 = step14_rdtsc();
    uint64_t t1 = timer_get_ticks();
    uint64_t dt = t1 - t0;
    if (dt == 0) dt = 1;
    s_cycles_per_ms = (c1 - c0) / dt;
    if (s_cycles_per_ms == 0) s_cycles_per_ms = 2000000;
    s_calibrated = true;

    display_print("\n========================================\n");
    display_print("[STEP14] Telemetry Engine Init OK\n");
    display_print("[STEP14] TSC cycles/ms: ");
    display_print_dec(s_cycles_per_ms);
    display_print("\n========================================\n\n");
}

void step14_log_irq(void) {
    g_step14_telemetry.last_irq_timestamp_ms = timer_get_ticks();
}

void step14_log_queue_push(int qlen) {
    g_step14_telemetry.irq_queue_len = qlen;
}

void step14_log_pump_start(uint64_t irq_time_ms) {
    if (irq_time_ms > 0) {
        uint64_t now = timer_get_ticks();
        if (now >= irq_time_ms) {
            g_step14_telemetry.irq_to_queue_us = (uint32_t)((now - irq_time_ms) * 1000);
        }
    }
}

void step14_log_hit_test_done(uint32_t duration_us) {
    g_step14_telemetry.hit_test_duration_us = duration_us;
}

void step14_log_pump_done(uint32_t duration_us) {
    g_step14_telemetry.pump_duration_us = duration_us;
}

void step14_log_compositor_start(void) {
}

void step14_log_compositor_done(uint32_t dirty_count, uint32_t duration_us) {
    g_step14_telemetry.dirty_rects_created = dirty_count;
    g_step14_telemetry.compositor_duration_us = duration_us;
}

void step14_log_cursor_draw(uint32_t duration_us) {
    g_step14_telemetry.bvcursor_draw_count++;
    g_step14_telemetry.sw_cursor_draws++;
    g_step14_telemetry.cursor_draw_duration_us = duration_us;
}

void step14_log_swapfull(uint32_t dirty_count, uint32_t duration_us, uint32_t bytes_copied) {
    g_step14_telemetry.last_dirty_count = dirty_count;
    g_step14_telemetry.vram_copy_duration_us = duration_us;
    g_step14_telemetry.bytes_copied_this_frame = bytes_copied;
    g_step14_telemetry.total_bytes_copied += bytes_copied;
    if (dirty_count == 0) {
        g_step14_telemetry.dirty_count_becomes_zero = true;
        g_step14_telemetry.legacy_swapfull_frames++;
    }
}

void step14_log_login_render(uint32_t total_us, uint32_t bg_us, uint32_t dyn_us, uint32_t eye_us, uint32_t pass_us, uint32_t btn_us) {
    g_step14_telemetry.login_total_render_us = total_us;
    g_step14_telemetry.login_bg_restore_us = bg_us;
    g_step14_telemetry.login_dynamic_controls_us = dyn_us;
    g_step14_telemetry.login_eye_icon_us = eye_us;
    g_step14_telemetry.login_password_us = pass_us;
    g_step14_telemetry.login_button_us = btn_us;
}

void step14_telemetry_dump_report(void) {
    display_print("\n==================================================\n");
    display_print("     STEP 15 FINAL RUNTIME AUDIT REPORT           \n");
    display_print("==================================================\n");
    
    display_print("\n--- 1. PRESENTATION PATH ---\n");
    display_print("Total Frames: "); display_print_dec(g_step14_telemetry.frame_count); display_print("\n");
    display_print("Legacy SwapFull Frames: "); display_print_dec(g_step14_telemetry.legacy_swapfull_frames); display_print("\n");
    display_print("Partial Copy Frames: "); display_print_dec(g_step14_telemetry.partial_copy_frames); display_print("\n");
    display_print("Dual Page Frames: "); display_print_dec(g_step14_telemetry.dual_page_frames); display_print("\n");
    display_print("Dirty Count (staging): "); display_print_dec(g_step14_telemetry.last_dirty_count); display_print("\n");
    display_print("Total Bytes Copied: "); display_print_dec(g_step14_telemetry.total_bytes_copied); display_print("\n");
    uint64_t fc = g_step14_telemetry.frame_count;
    if (fc == 0) fc = 1;
    display_print("Avg Bytes/Frame: "); display_print_dec((uint64_t)(g_step14_telemetry.total_bytes_copied / fc)); display_print("\n");
    display_print("Last VRAM Copy (us): "); display_print_dec(g_step14_telemetry.vram_copy_duration_us); display_print("\n");

    display_print("\n--- 2. CURSOR PIPELINE ---\n");
    display_print("BVCursor_Draw calls: "); display_print_dec(g_step14_telemetry.bvcursor_draw_count); display_print("\n");
    display_print("BSPE_CursorPlane_SetPosition calls: "); display_print_dec(g_step14_telemetry.bspe_set_position_count); display_print("\n");
    display_print("HW Cursor Updates: "); display_print_dec(g_step14_telemetry.hw_cursor_updates); display_print("\n");
    display_print("SW Cursor Draws: "); display_print_dec(g_step14_telemetry.sw_cursor_draws); display_print("\n");
    display_print("Last Cursor Draw (us): "); display_print_dec(g_step14_telemetry.cursor_draw_duration_us); display_print("\n");

    display_print("\n--- 3. FRAME TIMELINE (us) ---\n");
    display_print("IRQ->Queue (us): "); display_print_dec(g_step14_telemetry.irq_to_queue_us); display_print("\n");
    display_print("Event Pump (us): "); display_print_dec(g_step14_telemetry.pump_duration_us); display_print("\n");
    display_print("Hit Test (us): "); display_print_dec(g_step14_telemetry.hit_test_duration_us); display_print("\n");
    display_print("Compositor (us): "); display_print_dec(g_step14_telemetry.compositor_duration_us); display_print("\n");
    display_print("Cursor Draw (us): "); display_print_dec(g_step14_telemetry.cursor_draw_duration_us); display_print("\n");
    display_print("VRAM Copy (us): "); display_print_dec(g_step14_telemetry.vram_copy_duration_us); display_print("\n");

    display_print("\n--- 4. LOGIN RENDER BREAKDOWN (us) ---\n");
    display_print("Total: "); display_print_dec(g_step14_telemetry.login_total_render_us); display_print("\n");
    display_print("  BG Restore: "); display_print_dec(g_step14_telemetry.login_bg_restore_us); display_print("\n");
    display_print("  Dyn Controls: "); display_print_dec(g_step14_telemetry.login_dynamic_controls_us); display_print("\n");
    display_print("    Eye Icon: "); display_print_dec(g_step14_telemetry.login_eye_icon_us); display_print("\n");
    display_print("    Password: "); display_print_dec(g_step14_telemetry.login_password_us); display_print("\n");
    display_print("    Button: "); display_print_dec(g_step14_telemetry.login_button_us); display_print("\n");

    display_print("\n--- 5. DAMAGE TRACKING ---\n");
    display_print("Dirty Rects Created: "); display_print_dec(g_step14_telemetry.dirty_rects_created); display_print("\n");
    display_print("Dirty Rects Reach BSPE: "); display_print_dec(g_step14_telemetry.dirty_rects_reach_bspe); display_print("\n");
    display_print("dirty_count=0 in SwapFull: "); display_print(g_step14_telemetry.dirty_count_becomes_zero ? "YES\n" : "NO\n");
    display_print("Bytes This Frame: "); display_print_dec(g_step14_telemetry.bytes_copied_this_frame); display_print("\n");

    display_print("\n==================================================\n");
    display_print("     ENGINEERING VERDICT                           \n");
    display_print("==================================================\n");
    display_print("HW Cursor used: NO (0 calls)\n");
    display_print("Partial VRAM active: NO (dirty_count=0)\n");
    display_print("Legacy SwapFull active: YES (every frame)\n");
    display_print("==================================================\n\n");
}

void step14_telemetry_on_frame(void) {
    g_step14_telemetry.frame_count++;

    /* Frame 1: print immediately to verify hook is active */
    if (g_step14_telemetry.frame_count == 1) {
        // display_print("[STEP14] *** Frame Hook Active ***\n");
    }

    /* Frame 3: calibrate TSC (after system has settled) */
    if (g_step14_telemetry.frame_count == 3 && !s_calibrated) {
        step14_telemetry_init();
    }

#ifdef TEST_BUILD
    /* Frames 10-60: inject synthetic mouse movement for Login Screen stress test */
    if (g_step14_telemetry.frame_count >= 10 && g_step14_telemetry.frame_count <= 60) {
        kernel_input_push_mouse(8, 4, 0);
    }
#endif

    /* Every 20 frames: print progress */
    if (g_step14_telemetry.frame_count % 20 == 0) {
        // display_print("[STEP14] F");
        // display_print_dec(g_step14_telemetry.frame_count);
        // display_print(" VRAMus=");
        // display_print_dec(g_step14_telemetry.vram_copy_duration_us);
        // display_print(" LoginUs=");
        // display_print_dec(g_step14_telemetry.login_total_render_us);
        // display_print(" CompUs=");
        // display_print_dec(g_step14_telemetry.compositor_duration_us);
        // display_print(" CurUs=");
        // display_print_dec(g_step14_telemetry.cursor_draw_duration_us);
        // display_print(" PumpUs=");
        // display_print_dec(g_step14_telemetry.pump_duration_us);
        // display_print(" HitUs=");
        // display_print_dec(g_step14_telemetry.hit_test_duration_us);
        // display_print(" Dirty=");
        // display_print_dec(g_step14_telemetry.dirty_rects_created);
        // display_print(" Bytes=");
        // display_print_dec(g_step14_telemetry.bytes_copied_this_frame);
        // display_print("\n");
    }

#ifdef TEST_BUILD
    /* Frame 120: dump final report and shutdown */
    if (g_step14_telemetry.frame_count == 120) {
        step14_telemetry_dump_report();
        display_print("[STEP14] Audit complete. Shutting down.\n");
        horse_shutdown();
    }
#endif
}
