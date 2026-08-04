/*
 * BOSPECTRA V3 — Runtime Resource Graph Subsystem
 * kernel/media/bospectra/resource/resource_graph.h
 *
 * Maintains hierarchical ownership tree authority connecting parent owners to child resources.
 */

#ifndef BOSPECTRA_V3_RESOURCE_GRAPH_H
#define BOSPECTRA_V3_RESOURCE_GRAPH_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t node_id;
    uint32_t parent_node_id;
    char     name[32];
    bool     is_active;
} BOSPECTRA_GraphNode;

void              bospectra_resource_graph_init(void);
void              bospectra_resource_graph_shutdown(void);

bospectra_error_t bospectra_graph_add_node(uint32_t node_id, uint32_t parent_node_id, const char* name);
bospectra_error_t bospectra_graph_remove_node(uint32_t node_id);

void              bospectra_resource_graph_dump(void);

#endif /* BOSPECTRA_V3_RESOURCE_GRAPH_H */
