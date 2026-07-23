#ifndef ATOMS_GRAPH_BENCHMARK_H
#define ATOMS_GRAPH_BENCHMARK_H

#include <stdint.h>
#include <stdbool.h>
#include "atoms_graph_metrics.h"
#include "atoms_graph_renderer.h"

typedef struct {
    AtomsGraphMetrics  metrics;
    AtomsGraphRenderer renderer;
    
    bool is_running;
    bool is_finished;
    
    uint32_t current_stage;
    uint32_t stage_duration_ms; // Target ms per stage
    
    float animation_angle;
    uint32_t final_score;
} AtomsGraphBenchmark;

void atoms_graph_benchmark_init(AtomsGraphBenchmark* b, uint32_t win_id, uint32_t width, uint32_t height, uint32_t stage_duration_ms);
void atoms_graph_benchmark_start(AtomsGraphBenchmark* b);
bool atoms_graph_benchmark_step(AtomsGraphBenchmark* b, float delta_ms);
void atoms_graph_benchmark_cleanup(AtomsGraphBenchmark* b);
void atoms_graph_benchmark_dump_log(const AtomsGraphBenchmark* b);

#endif // ATOMS_GRAPH_BENCHMARK_H
