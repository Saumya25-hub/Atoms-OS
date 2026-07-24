#include "abe_layout_diag.h"
#include "../diagnostics/abe_diagnostics.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static bool g_layout_diag_initialized = false;

ABE_Error ABE_LayoutDiag_Init(void) {
    g_layout_diag_initialized = true;
    return ABE_SUCCESS;
}

ABE_Error ABE_LayoutDiag_Shutdown(void) {
    g_layout_diag_initialized = false;
    return ABE_SUCCESS;
}

static void DumpNodeRecursive(ABE_RenderNode* node, uint32_t depth) {
    if (!node) return;

    for (uint32_t i = 0; i < depth; i++) display_print("  ");
    display_print("[RenderNode] Handle: "); display_print_dec(node->handle);
    display_print(" Rect: ("); display_print_dec((uint32_t)node->content_box.x); display_print(", ");
    display_print_dec((uint32_t)node->content_box.y); display_print(", ");
    display_print_dec((uint32_t)node->content_box.width); display_print("x");
    display_print_dec((uint32_t)node->content_box.height); display_print(")\n");

    ABE_RenderNode* child = node->first_child;
    while (child) {
        DumpNodeRecursive(child, depth + 1);
        child = child->next_sibling;
    }
}

void ABE_LayoutDiag_DumpTree(ABE_RenderTreeHandle tree_handle) {
    if (!g_layout_diag_initialized || tree_handle == ABE_INVALID_HANDLE) return;
    ABE_RenderTree* tree = ABE_RenderTree_Get(tree_handle);
    if (!tree || !tree->root_node) return;

    display_print("=== ABE Layout Tree Dump ===\n");
    DumpNodeRecursive(tree->root_node, 0);
    display_print("============================\n");
}
