#include "atoms_graph_benchmark.h"
#include "atoms_graph_ui.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);
extern const BVFramebuffer* BWE_GetRenderTarget(void);


static const char* s_stage_names[10] = {
    "Baseline Geometry", "Geometry Scaling", "Depth Complexity",
    "Texture Workload", "Multi-Object Scene", "Render-to-Texture",
    "RTT Stress", "High Geometry Stress", "Combined Stress", "Stability Run"
};

void atoms_graph_benchmark_init(AtomsGraphBenchmark* b, uint32_t win_id, uint32_t width, uint32_t height, uint32_t stage_duration_ms) {
    if (!b) return;
    
    b->is_running = false;
    b->is_finished = false;
    b->current_stage = 0;
    b->stage_duration_ms = (stage_duration_ms > 0) ? stage_duration_ms : 30000; // default 30s per stage = 300s total (5 mins)
    b->animation_angle = 0.0f;
    b->final_score = 0;
    
    atoms_graph_renderer_init(&b->renderer, win_id, width, height);
    atoms_graph_metrics_init(&b->metrics, b->renderer.viewport_w, b->renderer.viewport_h);
}

void atoms_graph_benchmark_start(AtomsGraphBenchmark* b) {
    if (!b || !b->renderer.is_initialized) return;
    
    display_print("[ATOMS-GRAPH] Application started\n");
    display_print("[ATOMS-GRAPH] BGL context created\n");
    
    b->is_running = true;
    b->is_finished = false;
    b->current_stage = 0;
    b->animation_angle = 0.0f;
    
    atoms_graph_metrics_reset_baseline_memory(&b->metrics);
    
    atoms_graph_metrics_begin_stage(&b->metrics, 0, s_stage_names[0]);
    display_print("[ATOMS-GRAPH] Stage 1 started (Baseline Geometry)\n");
}

bool atoms_graph_benchmark_step(AtomsGraphBenchmark* b, float delta_ms) {
    if (!b || !b->is_running) return false;
    
    if (delta_ms <= 0.001f) delta_ms = 16.6f;
    
    b->animation_angle += delta_ms * 0.06f; // Smooth rotation
    if (b->animation_angle >= 360.0f) b->animation_angle -= 360.0f;
    
    uint32_t triangles = 0;
    uint32_t draw_calls = 0;
    
    bool rendered = atoms_graph_renderer_render_frame(&b->renderer, b->current_stage, b->animation_angle,
                                                       &triangles, &draw_calls);
    if (!rendered) return false;
    
    atoms_graph_metrics_update_frame(&b->metrics, delta_ms, triangles, draw_calls);
    
    // Draw 2D Side Panel Metrics UI overlay
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (fb) {
        BWE_Rect bounds = {0, 0, (int32_t)b->renderer.total_width, (int32_t)b->renderer.total_height};
        atoms_graph_ui_render_panel(fb, bounds, &b->metrics, b->is_finished, b->final_score);
        
        if (b->is_finished) {
            atoms_graph_ui_render_results_screen(fb, bounds, &b->metrics, b->final_score);
        }
    }
    
    // Check Stage Progression
    if (!b->is_finished && b->metrics.stage_elapsed_ms >= b->stage_duration_ms) {
        atoms_graph_metrics_end_stage(&b->metrics, b->current_stage);
        
        display_print("[ATOMS-GRAPH] Stage ");
        display_print_dec(b->current_stage + 1);
        display_print(" completed\n");
        
        b->current_stage++;
        if (b->current_stage < ATOMS_GRAPH_NUM_STAGES) {
            atoms_graph_metrics_begin_stage(&b->metrics, b->current_stage, s_stage_names[b->current_stage]);
            display_print("[ATOMS-GRAPH] Stage ");
            display_print_dec(b->current_stage + 1);
            display_print(" started (");
            display_print(s_stage_names[b->current_stage]);
            display_print(")\n");
        } else {
            b->is_finished = true;
            b->is_running = false;
            atoms_graph_metrics_generate_bottlenecks(&b->metrics);
            b->final_score = atoms_graph_metrics_compute_score(&b->metrics);
            
            atoms_graph_benchmark_dump_log(b);
        }
    }
    
    return true;
}

void atoms_graph_benchmark_cleanup(AtomsGraphBenchmark* b) {
    if (!b) return;
    atoms_graph_renderer_cleanup(&b->renderer);
    b->is_running = false;
}

void atoms_graph_benchmark_dump_log(const AtomsGraphBenchmark* b) {
    if (!b) return;
    display_print("[ATOMS-GRAPH] Benchmark completed\n");
    display_print("[ATOMS-GRAPH] Score: ");
    display_print_dec(b->final_score);
    display_print("\n");
    display_print("[ATOMS-GRAPH] Memory delta: ");
    display_print_dec((uint32_t)((b->metrics.mem_delta_kb >= 0) ? b->metrics.mem_delta_kb : -b->metrics.mem_delta_kb));
    display_print(" KB\n");
}
