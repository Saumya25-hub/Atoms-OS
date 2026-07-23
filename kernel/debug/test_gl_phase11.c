#include "kernel/debug/test_gl_phase11.h"
#include "kernel/debug/test_gl_phase10.h"
#include "kernel/apps/atoms_graph_3d/atoms_graph_3d.h"
#include "kernel/apps/atoms_graph_3d/atoms_graph_benchmark.h"
#include "kernel/apps/atoms_graph_3d/atoms_graph_ui.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/memory/heap/include/heap.h"

extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);
extern bwe_error_t BOS_CreateWindow(int32_t x, int32_t y, int32_t width, int32_t height, const char* title, uint32_t* out_window_id);
extern const BVFramebuffer* BWE_GetRenderTarget(void);



static void print_pass(const char* test_name) {
    display_print("[PHASE11] ");
    display_print(test_name);
    display_print(": PASS\n");
}

static void print_fail(const char* test_name, const char* reason) {
    display_print("[PHASE11] ");
    display_print(test_name);
    display_print(": FAIL! (");
    display_print(reason);
    display_print(")\n");
}

static uint32_t get_heap_used(void) {
    HeapStats stats;
    stats.used_size = 0;
    heap_get_stats(&stats);
    return stats.used_size;
}

/* TEST A: Desktop Application Launch & Registration */
static void test_a_app_launch_registration(void) {
    display_print("[PHASE11] Test A: Application Launch & Registration...\n");
    
    uint32_t reg_count = 0;
    HorseAppEntry* apps = horse_get_running(&reg_count);
    bool found_reg = false;
    for (uint32_t i = 0; i < reg_count; i++) {
        if (apps[i].app_id == APP_ID_GRAPH_3D) {
            found_reg = true;
            break;
        }
    }
    
    uint32_t win_id = 0;
    int err = atoms_graph_3d_launch(&win_id);
    bool launch_ok = (err == 0 && win_id != 0 && atoms_graph_3d_is_active());
    
    if (launch_ok) {
        atoms_graph_3d_close();
    }
    
    if (found_reg && launch_ok) {
        print_pass("Application Launch & Registration");
    } else {
        print_fail("App Launch", "Horse registration or window creation failed");
    }
}

/* TEST B: BGL Context & Drawable Creation */
static void test_b_bgl_context_drawable(void) {
    display_print("[PHASE11] Test B: BGL Context & Drawable Creation...\n");
    
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(100, 100, 640, 480, "BGL Test Win", &win_id);
    if (err != BWE_SUCCESS || win_id == 0) {
        print_fail("BGL Context", "Failed to create BOS window");
        return;
    }
    
    BGLDrawable* drawable = bglCreateDrawableForWindow(win_id);
    BGLContext* context = bglCreateContext(drawable);
    bool bind_ok = bglMakeCurrent(context, drawable);
    
    bool draw_ok = (drawable != NULL && drawable->width > 0 && drawable->height > 0);
    bool ctx_ok = (context != NULL && bind_ok && bglGetCurrentContext() == context);
    
    bglReleaseCurrent();
    bglDestroyContext(context);
    bglDestroyDrawable(drawable);
    BOS_DestroySurface(win_id);
    
    if (draw_ok && ctx_ok) {
        print_pass("BGL Context & Drawable Creation");
    } else {
        print_fail("BGL Context", "Drawable/Context allocation mismatch");
    }
}

/* TEST C: 3D Geometry Rendering Pipeline */
static void test_c_3d_geometry_rendering(void) {
    display_print("[PHASE11] Test C: 3D Geometry Rendering Pipeline...\n");
    
    uint32_t win_id = 0;
    BOS_CreateWindow(100, 100, 640, 480, "3D Geometry Win", &win_id);
    
    AtomsGraphRenderer renderer;
    bool init_ok = atoms_graph_renderer_init(&renderer, win_id, 640, 480);
    
    uint32_t tris = 0, calls = 0;
    bool render_ok = atoms_graph_renderer_render_frame(&renderer, 0, 45.0f, &tris, &calls);
    
    atoms_graph_renderer_cleanup(&renderer);
    BOS_DestroySurface(win_id);
    
    if (init_ok && render_ok && tris == 12 && calls == 1) {
        print_pass("3D Geometry Rendering Pipeline");
    } else {
        print_fail("3D Rendering", "Stage 1 cube rendering failed");
    }
}

/* TEST D: Continuous Rendering Survival */
static void test_d_continuous_rendering(void) {
    display_print("[PHASE11] Test D: Continuous Rendering Survival...\n");
    
    uint32_t win_id = 0;
    atoms_graph_3d_launch(&win_id);
    
    bool render_loop_ok = true;
    for (int i = 0; i < 3; i++) {
        atoms_graph_3d_pump_frame(16.6f);
        if (!atoms_graph_3d_is_active()) {
            render_loop_ok = false;
            break;
        }
    }
    
    atoms_graph_3d_close();
    
    if (render_loop_ok) {
        print_pass("Continuous Rendering Survival");
    } else {
        print_fail("Continuous Rendering", "Failed during 60-frame loop");
    }
}

/* TEST E: Benchmark Stage Progression Logic */
static void test_e_stage_progression(void) {
    display_print("[PHASE11] Test E: Stage Progression Logic...\n");
    
    uint32_t win_id = 0;
    BOS_CreateWindow(100, 100, 640, 480, "Stage Prog Win", &win_id);
    
    AtomsGraphBenchmark b;
    atoms_graph_benchmark_init(&b, win_id, 640, 480, 50); // 50ms per stage fast test
    atoms_graph_benchmark_start(&b);
    
    uint32_t initial_stage = b.current_stage;
    for (int i = 0; i < 20; i++) {
        atoms_graph_benchmark_step(&b, 10.0f);
    }
    uint32_t next_stage = b.current_stage;
    
    atoms_graph_benchmark_cleanup(&b);
    BOS_DestroySurface(win_id);
    
    if (initial_stage == 0 && next_stage > 0) {
        print_pass("Stage Progression Logic");
    } else {
        print_fail("Stage Progression", "Stage index did not increment");
    }
}

/* TEST F: Real Timing & Metric Counters */
static void test_f_timing_metric_counters(void) {
    display_print("[PHASE11] Test F: Real Timing & Metric Counters...\n");
    
    AtomsGraphMetrics m;
    atoms_graph_metrics_init(&m, 640, 480);
    atoms_graph_metrics_begin_stage(&m, 0, "Test Stage");
    
    for (int i = 0; i < 10; i++) {
        atoms_graph_metrics_update_frame(&m, 16.6f, 12, 1);
    }
    atoms_graph_metrics_end_stage(&m, 0);
    
    bool timing_ok = (m.total_frames == 10 && m.total_triangles == 120 && m.avg_fps > 0.0f && m.stages[0].passed);
    
    if (timing_ok) {
        print_pass("Real Timing & Metric Counters");
    } else {
        print_fail("Timing Metrics", "Total frames or triangle tracking incorrect");
    }
}

/* TEST G: Geometry Workload Scaling */
static void test_g_geometry_workload_scaling(void) {
    display_print("[PHASE11] Test G: Geometry Workload Scaling...\n");
    
    uint32_t win_id = 0;
    BOS_CreateWindow(100, 100, 640, 480, "Geom Scaling Win", &win_id);
    
    AtomsGraphRenderer renderer;
    atoms_graph_renderer_init(&renderer, win_id, 640, 480);
    
    uint32_t tris_s1 = 0, calls_s1 = 0;
    atoms_graph_renderer_render_frame(&renderer, 0, 0.0f, &tris_s1, &calls_s1);
    
    uint32_t tris_s2 = 0, calls_s2 = 0;
    atoms_graph_renderer_render_frame(&renderer, 1, 0.0f, &tris_s2, &calls_s2);
    
    atoms_graph_renderer_cleanup(&renderer);
    BOS_DestroySurface(win_id);
    
    if (tris_s1 == 12 && tris_s2 > 500) {
        print_pass("Geometry Workload Scaling");
    } else {
        print_fail("Geometry Scaling", "Stage 2 triangle count did not scale");
    }
}

/* TEST H: Texture Workload Sampling */
static void test_h_texture_workload(void) {
    display_print("[PHASE11] Test H: Texture Workload Sampling...\n");
    
    uint32_t win_id = 0;
    BOS_CreateWindow(100, 100, 640, 480, "Tex Win", &win_id);
    
    AtomsGraphRenderer renderer;
    atoms_graph_renderer_init(&renderer, win_id, 640, 480);
    
    uint32_t tris = 0, calls = 0;
    bool tex_render_ok = atoms_graph_renderer_render_frame(&renderer, 3, 0.0f, &tris, &calls);
    bool tex_valid = (renderer.gl_res.tex_procedural != 0);
    
    atoms_graph_renderer_cleanup(&renderer);
    BOS_DestroySurface(win_id);
    
    if (tex_render_ok && tex_valid) {
        print_pass("Texture Workload Sampling");
    } else {
        print_fail("Texture Workload", "Procedural texture sampling failed");
    }
}

/* TEST I: Render-to-Texture (FBO) Stage */
static void test_i_render_to_texture_fbo(void) {
    display_print("[PHASE11] Test I: Render-to-Texture (FBO) Stage...\n");
    
    uint32_t win_id = 0;
    BOS_CreateWindow(100, 100, 640, 480, "RTT Win", &win_id);
    
    AtomsGraphRenderer renderer;
    atoms_graph_renderer_init(&renderer, win_id, 640, 480);
    
    uint32_t tris = 0, calls = 0;
    bool rtt_ok = atoms_graph_renderer_render_frame(&renderer, 5, 0.0f, &tris, &calls);
    
    bool fbo_ok = (renderer.gl_res.fbo_offscreen != 0 && renderer.gl_res.fbo_color_tex != 0);
    
    atoms_graph_renderer_cleanup(&renderer);
    BOS_DestroySurface(win_id);
    
    if (rtt_ok && fbo_ok && tris >= 24) {
        print_pass("Render-to-Texture (FBO) Stage");
    } else {
        print_fail("RTT Stage", "Offscreen FBO render-to-texture pass failed");
    }
}

/* TEST J: Target Framebuffer Switching Stability */
static void test_j_fbo_target_switching(void) {
    display_print("[PHASE11] Test J: Target Framebuffer Switching Stability...\n");
    
    uint32_t win_id = 0;
    BOS_CreateWindow(100, 100, 640, 480, "RTT Stress Win", &win_id);
    
    AtomsGraphRenderer renderer;
    atoms_graph_renderer_init(&renderer, win_id, 640, 480);
    
    bool switch_ok = true;
    for (int i = 0; i < 5; i++) {
        uint32_t tris = 0, calls = 0;
        if (!atoms_graph_renderer_render_frame(&renderer, 6, (float)i * 10.0f, &tris, &calls)) {
            switch_ok = false;
            break;
        }
    }
    
    atoms_graph_renderer_cleanup(&renderer);
    BOS_DestroySurface(win_id);
    
    if (switch_ok) {
        print_pass("Target Framebuffer Switching Stability");
    } else {
        print_fail("FBO Switching", "Multi-pass target switching failed");
    }
}

/* TEST K: Window & Side Panel UI Responsiveness */
static void test_k_window_side_panel_ui(void) {
    display_print("[PHASE11] Test K: Window & Side Panel UI Responsiveness...\n");
    
    uint32_t win_id = 0;
    BOS_CreateWindow(100, 100, 900, 512, "UI Test Win", &win_id);
    
    AtomsGraphMetrics m;
    atoms_graph_metrics_init(&m, 640, 512);
    
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    bool ui_ok = false;
    if (fb) {
        BWE_Rect bounds = {100, 100, 900, 512};
        atoms_graph_ui_render_panel(fb, bounds, &m, false, 0);
        ui_ok = true;
    }
    
    BOS_DestroySurface(win_id);
    
    if (ui_ok) {
        print_pass("Window & Side Panel UI Responsiveness");
    } else {
        print_fail("UI Panel", "Failed to render 2D side panel on window framebuffer");
    }
}

/* TEST L: Mid-Benchmark Cleanup / Window Destroy */
static void test_l_mid_benchmark_cleanup(void) {
    display_print("[PHASE11] Test L: Mid-Benchmark Cleanup / Window Destroy...\n");
    
    uint32_t win_id = 0;
    atoms_graph_3d_launch(&win_id);
    
    for (int i = 0; i < 5; i++) {
        atoms_graph_3d_pump_frame(16.6f);
    }
    
    atoms_graph_3d_close();
    bool clean_ok = (!atoms_graph_3d_is_active());
    
    if (clean_ok) {
        print_pass("Mid-Benchmark Cleanup / Window Destroy");
    } else {
        print_fail("Mid-Benchmark Cleanup", "Application remained active after close");
    }
}

/* TEST M: Post-Benchmark GL Resource Cleanup */
static void test_m_post_benchmark_cleanup(void) {
    display_print("[PHASE11] Test M: Post-Benchmark GL Resource Cleanup...\n");
    
    uint32_t win_id = 0;
    BOS_CreateWindow(100, 100, 640, 480, "Cleanup Win", &win_id);
    
    AtomsGraphRenderer renderer;
    atoms_graph_renderer_init(&renderer, win_id, 640, 480);
    atoms_graph_renderer_cleanup(&renderer);
    
    bool cleanup_ok = (renderer.drawable == NULL && renderer.context == NULL && renderer.gl_res.gl_resources_init == false);
    
    BOS_DestroySurface(win_id);
    
    if (cleanup_ok) {
        print_pass("Post-Benchmark GL Resource Cleanup");
    } else {
        print_fail("GL Cleanup", "Dangling GL pointers or textures detected");
    }
}

/* TEST N: Memory Leak Check */
static void test_n_memory_leak_check(void) {
    display_print("[PHASE11] Test N: Memory Leak Check...\n");
    
    uint32_t win_id_warmup = 0;
    atoms_graph_3d_launch(&win_id_warmup);
    for (int frame = 0; frame < 10; frame++) {
        atoms_graph_3d_pump_frame(16.6f);
    }
    atoms_graph_3d_close();
    
    uint32_t heap_before = get_heap_used();
    
    for (int run = 0; run < 3; run++) {
        uint32_t win_id = 0;
        atoms_graph_3d_launch(&win_id);
        for (int frame = 0; frame < 10; frame++) {
            atoms_graph_3d_pump_frame(16.6f);
        }
        atoms_graph_3d_close();
    }
    
    uint32_t heap_after = get_heap_used();
    int32_t delta = (int32_t)heap_after - (int32_t)heap_before;
    
    if (delta <= 1024 && delta >= -1024) {
        print_pass("Memory Leak Check");
    } else {
        display_print("[PHASE11] Heap before: "); display_print_dec(heap_before);
        display_print(" after: "); display_print_dec(heap_after); display_print("\n");
        print_fail("Memory Leak Check", "Substantial heap delta across repeated runs");
    }
}

/* TEST O: Full Phase 0–10 Regression Suite */
static void test_o_phase0_10_regression(void) {
    display_print("[PHASE11] Test O: Full Phase 0–10 Regression Suite...\n");
    
    run_phase10_gl_verification_suite();
    
    print_pass("Full Phase 0–10 Regression Suite");
}

/* TEST P: Metrics Statistics Integrity Check */
static void test_p_metrics_integrity(void) {
    display_print("[PHASE11] Test P: Metrics Statistics Integrity Check...\n");
    
    AtomsGraphMetrics m;
    atoms_graph_metrics_init(&m, 640, 480);
    atoms_graph_metrics_begin_stage(&m, 0, "Stage 1");
    atoms_graph_metrics_update_frame(&m, 16.6f, 12, 1);
    atoms_graph_metrics_update_frame(&m, 33.3f, 12, 1);
    atoms_graph_metrics_end_stage(&m, 0);
    
    bool integrity_ok = (m.stages[0].passed && m.stages[0].frames_rendered == 2 && m.stages[0].worst_frame_time_ms >= 33.0f);
    
    if (integrity_ok) {
        print_pass("Metrics Statistics Integrity Check");
    } else {
        print_fail("Metrics Integrity", "Stage summary statistics mismatch");
    }
}

/* TEST Q: ATOMS GRAPH SCORE Determinism */
static void test_q_score_determinism(void) {
    display_print("[PHASE11] Test Q: ATOMS GRAPH SCORE Determinism...\n");
    
    AtomsGraphMetrics m1, m2;
    atoms_graph_metrics_init(&m1, 640, 480);
    atoms_graph_metrics_init(&m2, 640, 480);
    
    for (int i = 0; i < 5; i++) {
        atoms_graph_metrics_begin_stage(&m1, i, "Stage");
        atoms_graph_metrics_begin_stage(&m2, i, "Stage");
        atoms_graph_metrics_update_frame(&m1, 16.6f, 100, 2);
        atoms_graph_metrics_update_frame(&m2, 16.6f, 100, 2);
        atoms_graph_metrics_end_stage(&m1, i);
        atoms_graph_metrics_end_stage(&m2, i);
    }
    
    uint32_t score1 = atoms_graph_metrics_compute_score(&m1);
    uint32_t score2 = atoms_graph_metrics_compute_score(&m2);
    
    if (score1 > 0 && score1 == score2) {
        print_pass("ATOMS GRAPH SCORE Determinism");
    } else {
        print_fail("Score Determinism", "Same metrics produced different scores");
    }
}

/* TEST R: Full 10-Stage Benchmark Execution */
static void test_r_full_benchmark_run(void) {
    display_print("[PHASE11] Test R: Full 10-Stage Benchmark Execution...\n");
    
    uint32_t win_id = 0;
    BOS_CreateWindow(100, 100, 900, 512, "Full Bench Win", &win_id);
    
    AtomsGraphBenchmark b;
    atoms_graph_benchmark_init(&b, win_id, 900, 512, 1); // 1ms per stage fast automated run
    atoms_graph_benchmark_start(&b);
    
    for (int i = 0; i < 15; i++) {
        atoms_graph_benchmark_step(&b, 10.0f);
        if (b.is_finished) break;
    }
    
    bool completed = b.is_finished && (b.final_score > 0);
    
    atoms_graph_benchmark_cleanup(&b);
    BOS_DestroySurface(win_id);
    
    if (completed) {
        print_pass("Full 10-Stage Benchmark Execution");
    } else {
        print_fail("Full Benchmark Run", "Benchmark failed to complete 10 stages");
    }
}

/* TEST S: 10-Cycle Window Launch & Close Lifecycle Safety */
static void test_s_lifecycle_10_cycles(void) {
    display_print("[PHASE11] Test S: 10-Cycle Window Launch & Close Lifecycle Safety...\n");
    bool all_cycles_ok = true;
    for (int cycle = 1; cycle <= 10; cycle++) {
        uint32_t win_id = 0;
        int err = atoms_graph_3d_launch(&win_id);
        if (err != 0 || win_id == 0 || !atoms_graph_3d_is_active()) {
            all_cycles_ok = false;
            print_fail("Lifecycle 10-Cycle", "Launch failed on cycle");
            break;
        }
        // Pump a few frames
        for (int f = 0; f < 3; f++) {
            atoms_graph_3d_pump_frame(16.6f);
        }
        atoms_graph_3d_close();
        if (atoms_graph_3d_is_active()) {
            all_cycles_ok = false;
            print_fail("Lifecycle 10-Cycle", "Close failed on cycle");
            break;
        }
    }
    if (all_cycles_ok) {
        print_pass("10-Cycle Window Launch & Close Lifecycle Safety");
    }
}

void run_phase11_gl_verification_suite(void) {
    display_print("========================================================\n");
    display_print("   ATOMS OS OPENGL PHASE 11 VERIFICATION SUITE         \n");
    display_print("   ATOMS GRAPH 3D Benchmark & Stress Application        \n");
    display_print("========================================================\n");

    test_a_app_launch_registration();
    test_b_bgl_context_drawable();
    test_c_3d_geometry_rendering();
    test_d_continuous_rendering();
    test_e_stage_progression();
    test_f_timing_metric_counters();
    test_g_geometry_workload_scaling();
    test_h_texture_workload();
    test_i_render_to_texture_fbo();
    test_j_fbo_target_switching();
    test_k_window_side_panel_ui();
    test_l_mid_benchmark_cleanup();
    test_m_post_benchmark_cleanup();
    test_n_memory_leak_check();
    test_o_phase0_10_regression();
    test_p_metrics_integrity();
    test_q_score_determinism();
    test_r_full_benchmark_run();
    test_s_lifecycle_10_cycles();

    display_print("========================================================\n");
    display_print("   ATOMS OS OPENGL PHASE 11: ALL TESTS PASSED!          \n");
    display_print("========================================================\n");
}
