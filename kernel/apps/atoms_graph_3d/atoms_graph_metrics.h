#ifndef ATOMS_GRAPH_METRICS_H
#define ATOMS_GRAPH_METRICS_H

#include <stdint.h>
#include <stdbool.h>

#define ATOMS_GRAPH_NUM_STAGES 10

typedef struct {
    const char* stage_name;
    bool passed;
    float avg_fps;
    float min_fps;
    float avg_frame_time_ms;
    float worst_frame_time_ms;
    uint32_t peak_triangles;
    uint32_t peak_draw_calls;
    uint32_t duration_ms;
    uint64_t frames_rendered;
    uint32_t mem_before_kb;
    uint32_t mem_after_kb;
} AtomsGraphStageRecord;

typedef struct {
    // Current Real-time Metrics
    float current_fps;
    float avg_fps;
    float min_fps;
    float max_fps;
    
    float current_frame_time_ms;
    float avg_frame_time_ms;
    float worst_frame_time_ms;
    
    uint64_t total_frames;
    uint32_t current_triangles;
    uint64_t total_triangles;
    uint32_t draw_calls;
    
    uint32_t current_stage;
    uint64_t benchmark_start_ms;
    uint64_t stage_start_ms;
    uint64_t stage_elapsed_ms;
    uint64_t total_elapsed_ms;
    float progress_pct;
    
    uint32_t res_width;
    uint32_t res_height;
    
    uint32_t mem_start_kb;
    uint32_t mem_current_kb;
    int32_t  mem_delta_kb;
    
    // Per-stage statistics
    AtomsGraphStageRecord stages[ATOMS_GRAPH_NUM_STAGES];
    
    // Stage FPS accumulators
    uint64_t stage_frame_count;
    float    stage_accum_time_ms;
    float    accum_total_time_ms;
    float    stage_min_fps;
    float    stage_max_fps;
    float    stage_worst_ft_ms;
    
    // Bottleneck analysis findings
    int      largest_fps_drop_stage;
    float    largest_fps_drop_val;
    int      worst_ft_spike_stage;
    float    worst_ft_spike_val;
    uint32_t highest_workload_triangles;
} AtomsGraphMetrics;

void atoms_graph_metrics_init(AtomsGraphMetrics* m, uint32_t viewport_w, uint32_t viewport_h);
void atoms_graph_metrics_reset_baseline_memory(AtomsGraphMetrics* m);
void atoms_graph_metrics_begin_stage(AtomsGraphMetrics* m, uint32_t stage_idx, const char* name);
void atoms_graph_metrics_update_frame(AtomsGraphMetrics* m, float delta_ms, uint32_t triangles, uint32_t draw_calls);
void atoms_graph_metrics_end_stage(AtomsGraphMetrics* m, uint32_t stage_idx);
uint32_t atoms_graph_metrics_compute_score(const AtomsGraphMetrics* m);
void atoms_graph_metrics_generate_bottlenecks(AtomsGraphMetrics* m);

#endif // ATOMS_GRAPH_METRICS_H
