/*
 * BOSPECTRA V3 — Runtime Resource Graph Implementation
 * kernel/media/bospectra/resource/resource_graph.c
 */

#include "resource_graph.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_GRAPH_NODES 128

extern void display_print(const char* str);

static BOSPECTRA_GraphNode g_graph_nodes[BOSPECTRA_MAX_GRAPH_NODES];
static bool                g_graph_mgr_initialized = false;

void bospectra_resource_graph_init(void) {
    memset(g_graph_nodes, 0, sizeof(g_graph_nodes));
    g_graph_mgr_initialized = true;
    bospectra_log("RESOURCE_GRAPH", "BOSPECTRA V3 Runtime Resource Graph Initialized.");
}

void bospectra_resource_graph_shutdown(void) {
    g_graph_mgr_initialized = false;
}

bospectra_error_t bospectra_graph_add_node(uint32_t node_id, uint32_t parent_node_id, const char* name) {
    if (!g_graph_mgr_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    for (size_t i = 0; i < BOSPECTRA_MAX_GRAPH_NODES; i++) {
        if (!g_graph_nodes[i].is_active) {
            g_graph_nodes[i].node_id = node_id;
            g_graph_nodes[i].parent_node_id = parent_node_id;
            if (name) strncpy(g_graph_nodes[i].name, name, sizeof(g_graph_nodes[i].name) - 1);
            g_graph_nodes[i].is_active = true;
            return BOSPECTRA_SUCCESS;
        }
    }
    return BOSPECTRA_ERR_BUFFER_OVERFLOW;
}

bospectra_error_t bospectra_graph_remove_node(uint32_t node_id) {
    if (!g_graph_mgr_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    for (size_t i = 0; i < BOSPECTRA_MAX_GRAPH_NODES; i++) {
        if (g_graph_nodes[i].is_active && g_graph_nodes[i].node_id == node_id) {
            memset(&g_graph_nodes[i], 0, sizeof(BOSPECTRA_GraphNode));
            return BOSPECTRA_SUCCESS;
        }
    }
    return BOSPECTRA_ERR_STREAM_NOT_FOUND;
}

void bospectra_resource_graph_dump(void) {
    if (!g_graph_mgr_initialized) return;

    display_print("\n============= RESOURCE OWNERSHIP GRAPH =============\n");
    for (size_t i = 0; i < BOSPECTRA_MAX_GRAPH_NODES; i++) {
        if (g_graph_nodes[i].is_active) {
            display_print("Node [");
            bospectra_trace_u32("ID", g_graph_nodes[i].node_id);
            display_print("] Parent [");
            bospectra_trace_u32("ID", g_graph_nodes[i].parent_node_id);
            display_print("] ");
            display_print(g_graph_nodes[i].name);
            display_print("\n");
        }
    }
    display_print("====================================================\n");
}
