#include "abe_reflow.h"
#include "abe_block_layout.h"
#include "abe_positioning.h"
#include "../diagnostics/abe_diagnostics.h"

static bool g_reflow_initialized = false;

ABE_Error ABE_Reflow_Init(void) {
    g_reflow_initialized = true;
    ABE_Log(ABE_LOG_INFO, "REFLOW", "ABE Incremental Reflow Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_Reflow_Shutdown(void) {
    g_reflow_initialized = false;
    return ABE_SUCCESS;
}

static uint32_t CountDirtyNodesRecursive(ABE_RenderNode* node) {
    if (!node) return 0;
    uint32_t count = node->is_dirty ? 1 : 0;

    ABE_RenderNode* child = node->first_child;
    while (child) {
        count += CountDirtyNodesRecursive(child);
        child = child->next_sibling;
    }
    return count;
}

ABE_Error ABE_Reflow_Perform(ABE_RenderTreeHandle tree_handle) {
    if (!g_reflow_initialized || tree_handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_RenderTree* tree = ABE_RenderTree_Get(tree_handle);
    if (!tree || !tree->root_node) return ABE_ERR_RENDER_TREE_FAILED;

    uint32_t dirty_count = CountDirtyNodesRecursive(tree->root_node);
    if (dirty_count == 0) {
        return ABE_SUCCESS; // Clean, no reflow needed
    }

    // Perform Layout on Root
    ABE_BlockLayout_Perform(tree->root_node, tree->viewport_width, tree->viewport_height);
    ABE_Positioning_Apply(tree->root_node, tree->viewport_width, tree->viewport_height);
    ABE_Overflow_Compute(tree->root_node);

    ABE_Diag_RecordReflow(dirty_count);
    ABE_LogVal(ABE_LOG_INFO, "REFLOW", "Incremental Reflow completed, Dirty nodes processed: ", dirty_count);
    return ABE_SUCCESS;
}

ABE_Error ABE_Reflow_MarkNodeDirty(ABE_RenderTreeHandle tree_handle, ABE_NodeHandle dom_handle) {
    if (!g_reflow_initialized || tree_handle == ABE_INVALID_HANDLE || dom_handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_RenderTree* tree = ABE_RenderTree_Get(tree_handle);
    if (!tree || !tree->root_node) return ABE_ERR_RENDER_TREE_FAILED;

    ABE_RenderNode* rnode = ABE_RenderTree_FindNodeByDOMHandle(tree->root_node, dom_handle);
    if (!rnode) return ABE_ERR_LAYOUT_BOX_NOT_FOUND;

    // Mark node and ancestors dirty up to root
    ABE_RenderNode* curr = rnode;
    while (curr) {
        curr->is_dirty = true;
        curr = curr->parent;
    }
    return ABE_SUCCESS;
}
