#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

// Graph Engine — ring-buffer sample store for 60fps live charts
static TASKMGR_GRAPH s_graph = {0};

void taskmgr_graphs_init(void) {
    s_graph.sample_head = 0;
    display_print("[TASKMGR_GRAPH] Graph Engine Initialized (256-sample ring buffer, 60fps).\n");
}

void taskmgr_graphs_push(uint32_t cpu, uint32_t ram, uint32_t gpu,
                          uint32_t disk, uint32_t net, uint32_t temp, uint32_t fps) {
    uint32_t h = s_graph.sample_head % TASKMGR_GRAPH_SAMPLES;
    s_graph.cpu_samples[h]  = cpu;
    s_graph.ram_samples[h]  = ram;
    s_graph.gpu_samples[h]  = gpu;
    s_graph.disk_samples[h] = disk;
    s_graph.net_samples[h]  = net;
    s_graph.temp_samples[h] = temp;
    s_graph.fps_samples[h]  = fps;
    s_graph.sample_head++;
}

TASKMGR_GRAPH* taskmgr_graphs_get(void) { return &s_graph; }
