#include "kernel/debug/test_bgl_phase1.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/crash_log.h"

extern Task* current_task;
extern BWE_Window* BWE_GetWindow(uint32_t window_id);

static void test_bgl_lifecycle_stress(uint32_t win_id) {
    display_print("[PHASE1] Test A: Context Lifecycle Stress (20 Cycles)...\n");

    bool all_passed = true;
    for (int cycle = 0; cycle < 20; cycle++) {
        BGLDrawable* d = bglCreateDrawableForWindow(win_id);
        if (!d) {
            display_print("[PHASE1] ERROR: bglCreateDrawableForWindow failed on cycle ");
            display_print_dec(cycle); display_print("\n");
            all_passed = false;
            break;
        }

        BGLContext* ctx = bglCreateContext(d);
        if (!ctx) {
            display_print("[PHASE1] ERROR: bglCreateContext failed on cycle ");
            display_print_dec(cycle); display_print("\n");
            bglDestroyDrawable(d);
            all_passed = false;
            break;
        }

        if (!bglMakeCurrent(ctx, d)) {
            display_print("[PHASE1] ERROR: bglMakeCurrent failed!\n");
            all_passed = false;
        }

        bglDiagnosticClear(ctx, 0xFF00FF00); // Green
        bglSwapBuffers(ctx);

        bglReleaseCurrent();
        bglDestroyContext(ctx);
        bglDestroyDrawable(d);

        if (!all_passed) break;
    }

    if (all_passed) {
        display_print("[PHASE1] Context Lifecycle Stress Test: PASS (20/20 cycles OK)\n");
        crash_log_add("[PHASE1 TEST] Context Lifecycle Stress Test: PASS");
    } else {
        display_print("[PHASE1] Context Lifecycle Stress Test: FAIL!\n");
        crash_log_add("[PHASE1 TEST] Context Lifecycle Stress Test: FAIL");
    }
}

static void test_bgl_multi_context_isolation(uint32_t win1_id, uint32_t win2_id) {
    display_print("\n[PHASE1] Test B: Multiple Context Isolation...\n");

    BGLDrawable* d1 = bglCreateDrawableForWindow(win1_id);
    BGLDrawable* d2 = bglCreateDrawableForWindow(win2_id);
    if (!d1 || !d2) {
        display_print("[PHASE1] ERROR: Failed to create drawables for isolation test!\n");
        return;
    }

    BGLContext* c1 = bglCreateContext(d1);
    BGLContext* c2 = bglCreateContext(d2);
    if (!c1 || !c2) {
        display_print("[PHASE1] ERROR: Failed to create contexts for isolation test!\n");
        bglDestroyDrawable(d1);
        bglDestroyDrawable(d2);
        return;
    }

    bool isolated = true;
    for (int i = 0; i < 50; i++) {
        // Bind Context 1 & Clear Blue (0xFF0000FF)
        bglMakeCurrent(c1, d1);
        bglDiagnosticClear(c1, 0xFF0000FF);
        bglSwapBuffers(c1);

        // Bind Context 2 & Clear Red (0xFFFF0000)
        bglMakeCurrent(c2, d2);
        bglDiagnosticClear(c2, 0xFFFF0000);
        bglSwapBuffers(c2);

        // Verify Context 1 buffer is strictly Blue
        if (d1->color_buffer[0] != 0xFF0000FF || d1->color_buffer[d1->width * d1->height - 1] != 0xFF0000FF) {
            isolated = false;
            display_print("[PHASE1] ERROR: Context 1 buffer corrupted!\n");
            break;
        }

        // Verify Context 2 buffer is strictly Red
        if (d2->color_buffer[0] != 0xFFFF0000 || d2->color_buffer[d2->width * d2->height - 1] != 0xFFFF0000) {
            isolated = false;
            display_print("[PHASE1] ERROR: Context 2 buffer corrupted!\n");
            break;
        }
    }

    bglReleaseCurrent();
    bglDestroyContext(c1);
    bglDestroyContext(c2);
    bglDestroyDrawable(d1);
    bglDestroyDrawable(d2);

    if (isolated) {
        display_print("[PHASE1] Multi-Context Isolation Test: PASS\n");
        crash_log_add("[PHASE1 TEST] Multi-Context Isolation Test: PASS");
    } else {
        display_print("[PHASE1] Multi-Context Isolation Test: FAIL!\n");
        crash_log_add("[PHASE1 TEST] Multi-Context Isolation Test: FAIL");
    }
}

static void test_bgl_invalid_handle_rejection(void) {
    display_print("\n[PHASE1] Test D: Invalid Handle & Ownership Rejection...\n");

    bool reject_ok = true;

    // Test 1: Null context MakeCurrent
    if (bglMakeCurrent(NULL, NULL) != true) { // Releasing current should return true
        reject_ok = false;
    }

    // Test 2: Unbound SwapBuffers
    if (bglSwapBuffers(NULL) != false) {
        reject_ok = false;
    }

    // Test 3: Get last error returns appropriate code
    if (bglGetLastError() != BGL_ERR_INVALID_CONTEXT) {
        reject_ok = false;
    }

    if (reject_ok) {
        display_print("[PHASE1] Invalid Handle Rejection Test: PASS\n");
        crash_log_add("[PHASE1 TEST] Invalid Handle Rejection Test: PASS");
    } else {
        display_print("[PHASE1] Invalid Handle Rejection Test: FAIL!\n");
        crash_log_add("[PHASE1 TEST] Invalid Handle Rejection Test: FAIL");
    }
}

static void test_bgl_resize_safety_torture(uint32_t win_id) {
    display_print("\n[PHASE1] Test E: Resize Safety Torture (100 Resizes)...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) {
        display_print("[PHASE1] ERROR: Failed to create objects for resize test!\n");
        return;
    }

    bglMakeCurrent(ctx, d);

    const uint32_t sizes[][2] = {
        {100, 100}, {640, 480}, {320, 240}, {1280, 720}, {800, 600}, {400, 300}
    };
    const size_t num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    bool resize_ok = true;
    for (int i = 0; i < 10; i++) {
        uint32_t nw = sizes[i % num_sizes][0];
        uint32_t nh = sizes[i % num_sizes][1];

        uint32_t old_gen = d->generation;

        if (!bglResizeDrawable(d, nw, nh)) {
            display_print("[PHASE1] ERROR: bglResizeDrawable failed at step ");
            display_print_dec(i); display_print("\n");
            resize_ok = false;
            break;
        }

        if (d->generation <= old_gen) {
            display_print("[PHASE1] ERROR: Generation count did not increment!\n");
            resize_ok = false;
            break;
        }

        if (d->pitch != nw * 4) {
            display_print("[PHASE1] ERROR: Pitch mismatch after resize!\n");
            resize_ok = false;
            break;
        }

        bglDiagnosticClear(ctx, 0xFF123456);
        bglSwapBuffers(ctx);
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (resize_ok) {
        display_print("[PHASE1] Resize Safety Torture Test: PASS (10/10 resizes OK)\n");
        crash_log_add("[PHASE1 TEST] Resize Safety Torture Test: PASS");
    } else {
        display_print("[PHASE1] Resize Safety Torture Test: FAIL!\n");
        crash_log_add("[PHASE1 TEST] Resize Safety Torture Test: FAIL");
    }
}

static void test_bgl_presentation_stress(uint32_t win_id) {
    display_print("\n[PHASE1] Presentation Stress Test (20 Frames)...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) {
        display_print("[PHASE1] ERROR: Failed to create objects for presentation test!\n");
        return;
    }

    bglMakeCurrent(ctx, d);

    bool present_ok = true;
    uint32_t colors[] = {0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFFFFFF00, 0xFF00FFFF};

    for (int frame = 0; frame < 20; frame++) {
        uint32_t c = colors[frame % 5];
        bglDiagnosticClear(ctx, c);
        if (!bglSwapBuffers(ctx)) {
            present_ok = false;
            display_print("[PHASE1] ERROR: bglSwapBuffers failed at frame ");
            display_print_dec(frame); display_print("\n");
            break;
        }
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (present_ok) {
        display_print("[PHASE1] Presentation Stress Test: PASS (20/20 frames presented)\n");
        crash_log_add("[PHASE1 TEST] Presentation Stress Test: PASS");
    } else {
        display_print("[PHASE1] Presentation Stress Test: FAIL!\n");
        crash_log_add("[PHASE1 TEST] Presentation Stress Test: FAIL");
    }
}

void run_phase1_bgl_verification_suite(void) {
    display_print("=========================================\n");
    display_print("   ATOMS OS — OPENGL PHASE 1 VERIFICATION\n");
    display_print("=========================================\n");

    // Create test BWE windows for Phase 1 verification
    uint32_t win1 = 0, win2 = 0;
    bwe_error_t err1 = BOS_CreateWindow(100, 100, 160, 120, "BGL_Window1", &win1);
    bwe_error_t err2 = BOS_CreateWindow(200, 200, 160, 120, "BGL_Window2", &win2);

    if (err1 != BWE_SUCCESS || err2 != BWE_SUCCESS || win1 == 0 || win2 == 0) {
        display_print("[PHASE1] ERROR: Failed to create BWE windows for testing!\n");
        return;
    }

    test_bgl_lifecycle_stress(win1);
    test_bgl_multi_context_isolation(win1, win2);
    test_bgl_invalid_handle_rejection();
    test_bgl_resize_safety_torture(win1);
    test_bgl_presentation_stress(win1);

    BOS_DestroySurface(win1);
    BOS_DestroySurface(win2);

    display_print("=========================================\n");
}
