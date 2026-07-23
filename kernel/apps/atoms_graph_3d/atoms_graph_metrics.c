#include "atoms_graph_metrics.h"
#include "kernel/core/memory/heap/include/heap.h"

extern uint64_t timer_get_ticks(void);

static uint32_t get_heap_used_kb(void) {
    HeapStats stats;
    stats.used_size = 0;
    heap_get_stats(&stats);
    return stats.used_size / 1024;
}

void atoms_graph_metrics_init(AtomsGraphMetrics* m, uint32_t viewport_w, uint32_t viewport_h) {
    if (!m) return;
    
    m->current_fps = 0.0f;
    m->avg_fps = 0.0f;
    m->min_fps = 9999.0f;
    m->max_fps = 0.0f;
    
    m->current_frame_time_ms = 0.0f;
    m->avg_frame_time_ms = 0.0f;
    m->worst_frame_time_ms = 0.0f;
    
    m->total_frames = 0;
    m->current_triangles = 0;
    m->total_triangles = 0;
    m->draw_calls = 0;
    
    m->current_stage = 0;
    m->benchmark_start_ms = timer_get_ticks();
    m->stage_start_ms = m->benchmark_start_ms;
    m->stage_elapsed_ms = 0;
    m->total_elapsed_ms = 0;
    m->progress_pct = 0.0f;
    
    m->res_width = viewport_w;
    m->res_height = viewport_h;
    
    m->mem_start_kb = get_heap_used_kb();
    m->mem_current_kb = m->mem_start_kb;
    m->mem_delta_kb = 0;
    
    m->stage_frame_count = 0;
    m->stage_accum_time_ms = 0.0f;
    m->accum_total_time_ms = 0.0f;
    m->stage_min_fps = 9999.0f;
    m->stage_max_fps = 0.0f;
    m->stage_worst_ft_ms = 0.0f;
    
    m->largest_fps_drop_stage = -1;
    m->largest_fps_drop_val = 0.0f;
    m->worst_ft_spike_stage = -1;
    m->worst_ft_spike_val = 0.0f;
    m->highest_workload_triangles = 0;
    
    for (int i = 0; i < ATOMS_GRAPH_NUM_STAGES; i++) {
        m->stages[i].stage_name = "";
        m->stages[i].passed = false;
        m->stages[i].avg_fps = 0.0f;
        m->stages[i].min_fps = 0.0f;
        m->stages[i].avg_frame_time_ms = 0.0f;
        m->stages[i].worst_frame_time_ms = 0.0f;
        m->stages[i].peak_triangles = 0;
        m->stages[i].peak_draw_calls = 0;
        m->stages[i].duration_ms = 0;
        m->stages[i].frames_rendered = 0;
        m->stages[i].mem_before_kb = 0;
        m->stages[i].mem_after_kb = 0;
    }
}

void atoms_graph_metrics_reset_baseline_memory(AtomsGraphMetrics* m) {
    if (!m) return;
    m->mem_start_kb = get_heap_used_kb();
    m->mem_current_kb = m->mem_start_kb;
    m->mem_delta_kb = 0;
}

void atoms_graph_metrics_begin_stage(AtomsGraphMetrics* m, uint32_t stage_idx, const char* name) {
    if (!m || stage_idx >= ATOMS_GRAPH_NUM_STAGES) return;
    
    m->current_stage = stage_idx;
    m->stage_start_ms = timer_get_ticks();
    m->stage_elapsed_ms = 0;
    
    m->stage_frame_count = 0;
    m->stage_accum_time_ms = 0.0f;
    m->stage_min_fps = 9999.0f;
    m->stage_max_fps = 0.0f;
    m->stage_worst_ft_ms = 0.0f;
    
    m->stages[stage_idx].stage_name = name;
    m->stages[stage_idx].mem_before_kb = get_heap_used_kb();
}

void atoms_graph_metrics_update_frame(AtomsGraphMetrics* m, float delta_ms, uint32_t triangles, uint32_t draw_calls) {
    if (!m) return;
    
    if (delta_ms <= 0.001f) delta_ms = 0.001f;
    
    m->total_frames++;
    m->stage_frame_count++;
    
    m->current_frame_time_ms = delta_ms;
    float instant_fps = 1000.0f / delta_ms;
    m->current_fps = instant_fps;
    
    if (instant_fps < m->min_fps) m->min_fps = instant_fps;
    if (instant_fps > m->max_fps) m->max_fps = instant_fps;
    if (delta_ms > m->worst_frame_time_ms) m->worst_frame_time_ms = delta_ms;
    
    if (instant_fps < m->stage_min_fps) m->stage_min_fps = instant_fps;
    if (instant_fps > m->stage_max_fps) m->stage_max_fps = instant_fps;
    if (delta_ms > m->stage_worst_ft_ms) m->stage_worst_ft_ms = delta_ms;
    
    m->stage_accum_time_ms += delta_ms;
    m->accum_total_time_ms += delta_ms;
    
    uint64_t timer_elapsed = timer_get_ticks() - m->benchmark_start_ms;
    m->total_elapsed_ms = (timer_elapsed > 0) ? timer_elapsed : (uint64_t)m->accum_total_time_ms;
    
    uint64_t stage_timer_elapsed = timer_get_ticks() - m->stage_start_ms;
    m->stage_elapsed_ms = (stage_timer_elapsed > 0) ? stage_timer_elapsed : (uint64_t)m->stage_accum_time_ms;
    
    m->avg_frame_time_ms = (m->total_frames > 0) ? ((float)m->accum_total_time_ms / (float)m->total_frames) : 0.0f;
    m->avg_fps = (m->avg_frame_time_ms > 0.001f) ? (1000.0f / m->avg_frame_time_ms) : 0.0f;
    
    m->current_triangles = triangles;
    m->total_triangles += triangles;
    m->draw_calls = draw_calls;
    
    if (triangles > m->highest_workload_triangles) {
        m->highest_workload_triangles = triangles;
    }
    
    m->mem_current_kb = get_heap_used_kb();
    m->mem_delta_kb = (int32_t)m->mem_current_kb - (int32_t)m->mem_start_kb;
}

void atoms_graph_metrics_end_stage(AtomsGraphMetrics* m, uint32_t stage_idx) {
    if (!m || stage_idx >= ATOMS_GRAPH_NUM_STAGES) return;
    
    AtomsGraphStageRecord* st = &m->stages[stage_idx];
    st->passed = true;
    st->duration_ms = (uint32_t)m->stage_elapsed_ms;
    if (st->duration_ms == 0) st->duration_ms = (uint32_t)m->stage_accum_time_ms;
    st->frames_rendered = m->stage_frame_count;
    
    if (st->duration_ms > 0 && m->stage_frame_count > 0) {
        st->avg_frame_time_ms = (float)st->duration_ms / (float)m->stage_frame_count;
        st->avg_fps = (st->avg_frame_time_ms > 0.001f) ? (1000.0f / st->avg_frame_time_ms) : 0.0f;
    } else {
        st->avg_frame_time_ms = 0.0f;
        st->avg_fps = 0.0f;
    }
    
    st->min_fps = (m->stage_min_fps < 9900.0f) ? m->stage_min_fps : st->avg_fps;
    st->worst_frame_time_ms = m->stage_worst_ft_ms;
    st->peak_triangles = m->current_triangles;
    st->peak_draw_calls = m->draw_calls;
    st->mem_after_kb = get_heap_used_kb();
}

void atoms_graph_metrics_generate_bottlenecks(AtomsGraphMetrics* m) {
    if (!m) return;
    
    float max_fps_drop = 0.0f;
    int fps_drop_stage = 0;
    
    float worst_spike = 0.0f;
    int spike_stage = 0;
    
    float baseline_fps = m->stages[0].avg_fps;
    
    for (int i = 0; i < ATOMS_GRAPH_NUM_STAGES; i++) {
        if (!m->stages[i].passed) continue;
        
        float drop = baseline_fps - m->stages[i].avg_fps;
        if (drop > max_fps_drop) {
            max_fps_drop = drop;
            fps_drop_stage = i;
        }
        
        if (m->stages[i].worst_frame_time_ms > worst_spike) {
            worst_spike = m->stages[i].worst_frame_time_ms;
            spike_stage = i;
        }
    }
    
    m->largest_fps_drop_stage = fps_drop_stage;
    m->largest_fps_drop_val = max_fps_drop;
    m->worst_ft_spike_stage = spike_stage;
    m->worst_ft_spike_val = worst_spike;
}

uint32_t atoms_graph_metrics_compute_score(const AtomsGraphMetrics* m) {
    if (!m) return 0;
    
    float fps_component = m->avg_fps * 100.0f;
    float tri_component = (float)m->total_triangles / 1000.0f;
    
    uint32_t passed_count = 0;
    for (int i = 0; i < ATOMS_GRAPH_NUM_STAGES; i++) {
        if (m->stages[i].passed) passed_count++;
    }
    float stage_component = (float)passed_count * 500.0f;
    
    float ft_penalty = m->worst_frame_time_ms * 10.0f;
    float mem_penalty = (m->mem_delta_kb > 0) ? ((float)m->mem_delta_kb * 2.0f) : 0.0f;
    
    float total_score = (fps_component + tri_component + stage_component) - (ft_penalty + mem_penalty);
    if (total_score < 0.0f) total_score = 0.0f;
    
    return (uint32_t)total_score;
}
