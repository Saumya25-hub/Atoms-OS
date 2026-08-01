#include "explorer_treeview.h"
#include "kernel/core/lib/include/string.h"

void explorer_treeview_init(ExplorerTreeView* tv) {
    if (!tv) return;
    memset(tv, 0, sizeof(ExplorerTreeView));
    tv->selected_node_idx = 0;
    explorer_treeview_populate_roots(tv);
}

void explorer_treeview_populate_roots(ExplorerTreeView* tv) {
    if (!tv) return;
    tv->node_count = 0;
    
    // Add "This PC"
    strcpy(tv->nodes[0].name, "This PC");
    strcpy(tv->nodes[0].path, "/");
    tv->nodes[0].is_expanded = true;
    tv->nodes[0].has_children = true;
    tv->nodes[0].depth = 0;
    tv->node_count++;
    
    // Child roots
    const char* roots[][2] = {
        {"Desktop", "/DESKTOP"},
        {"Documents", "/DOCS"},
        {"Downloads", "/DOWNLOAD"},
        {"Music", "/MUSIC"},
        {"Pictures", "/PHOTO"},
        {"Videos", "/VIDEO"},
        {"USB Drive (U:)", "U:\\"}
    };
    
    for (int i = 0; i < 7; i++) {
        ExplorerTreeNode* node = &tv->nodes[tv->node_count++];
        strcpy(node->name, roots[i][0]);
        strcpy(node->path, roots[i][1]);
        node->is_expanded = false;
        node->has_children = false;
        node->depth = 1;
    }
}

bool explorer_treeview_toggle(ExplorerTreeView* tv, int32_t node_idx) {
    if (!tv || node_idx < 0 || (uint32_t)node_idx >= tv->node_count) return false;
    tv->nodes[node_idx].is_expanded = !tv->nodes[node_idx].is_expanded;
    tv->selected_node_idx = node_idx;
    return true;
}

const char* explorer_treeview_get_path(const ExplorerTreeView* tv, int32_t node_idx) {
    if (!tv || node_idx < 0 || (uint32_t)node_idx >= tv->node_count) return "/";
    return tv->nodes[node_idx].path;
}
