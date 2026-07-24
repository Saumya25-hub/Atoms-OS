#ifndef ADF_SNAPSHOT_H
#define ADF_SNAPSHOT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char     current_url[256];
    uint32_t dom_nodes_active;
    uint32_t js_contexts_active;
    uint32_t gpu_textures_allocated;
    uint32_t gpu_shaders_compiled;
    uint32_t layout_tree_depth;
    uint32_t heap_allocated_bytes;
} ABE_CrashSnapshot;

typedef struct {
    uint32_t gpu_leaks;
    uint32_t texture_leaks;
    uint32_t shader_leaks;
    uint32_t dom_leaks;
    uint32_t js_leaks;
    uint32_t canvas_leaks;
    uint32_t wasm_leaks;
    uint32_t memory_leaks;
} ABE_LeakReport;

typedef struct {
    uint32_t fps;
    uint32_t gpu_time_us;
    uint32_t paint_time_us;
    uint32_t layout_time_us;
    uint32_t js_time_us;
    uint32_t gc_time_us;
    uint32_t memory_used_kb;
    uint32_t texture_count;
    uint32_t buffer_count;
    uint32_t shader_count;
    uint32_t dom_node_count;
    uint32_t render_node_count;
} ABE_PerformanceCounters;

void                    ABE_DebugTakeSnapshot(const char* url, ABE_CrashSnapshot* out_snapshot);
ABE_LeakReport          ABE_DebugReportLeaks(void);
ABE_PerformanceCounters ABE_DebugGetPerformanceCounters(void);

#ifdef __cplusplus
}
#endif

#endif // ADF_SNAPSHOT_H
