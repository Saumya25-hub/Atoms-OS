#ifndef BSEC_EXPLORER_TREEVIEW_H
#define BSEC_EXPLORER_TREEVIEW_H

#include <stdint.h>
#include <stdbool.h>

#define BSEC_TREEVIEW_MAX_NODES 64

typedef struct {
    char    name[64];
    char    path[256];
    bool    is_expanded;
    bool    has_children;
    int32_t depth;
} ExplorerTreeNode;

typedef struct {
    ExplorerTreeNode nodes[BSEC_TREEVIEW_MAX_NODES];
    uint32_t         node_count;
    int32_t          selected_node_idx;
} ExplorerTreeView;

void explorer_treeview_init(ExplorerTreeView* tv);
void explorer_treeview_populate_roots(ExplorerTreeView* tv);
bool explorer_treeview_toggle(ExplorerTreeView* tv, int32_t node_idx);
const char* explorer_treeview_get_path(const ExplorerTreeView* tv, int32_t node_idx);

#endif // BSEC_EXPLORER_TREEVIEW_H
