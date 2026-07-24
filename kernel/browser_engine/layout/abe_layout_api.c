#include "../../../sdk/include/abe/abe.h"
#include "abe_render_tree.h"
#include "abe_box_model.h"
#include "abe_block_layout.h"
#include "abe_inline_layout.h"
#include "abe_flex_layout.h"
#include "abe_positioning.h"
#include "abe_reflow.h"
#include "abe_layout_diag.h"
#include "../diagnostics/abe_diagnostics.h"

static bool g_layout_api_initialized = false;

ABE_Error ABE_LayoutInitialize(void) {
    if (g_layout_api_initialized) return ABE_ERR_ALREADY_INITIALIZED;

    ABE_RenderTree_Init();
    ABE_BoxModel_Init();
    ABE_BlockLayout_Init();
    ABE_InlineLayout_Init();
    ABE_FlexLayout_Init();
    ABE_Positioning_Init();
    ABE_Reflow_Init();
    ABE_LayoutDiag_Init();

    g_layout_api_initialized = true;
    ABE_Log(ABE_LOG_INFO, "LAYOUTAPI", "ABE Layout Engine Public API Bridge initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_LayoutShutdown(void) {
    if (!g_layout_api_initialized) return ABE_ERR_NOT_INITIALIZED;

    ABE_LayoutDiag_Shutdown();
    ABE_Reflow_Shutdown();
    ABE_Positioning_Shutdown();
    ABE_FlexLayout_Shutdown();
    ABE_InlineLayout_Shutdown();
    ABE_BlockLayout_Shutdown();
    ABE_BoxModel_Shutdown();
    ABE_RenderTree_Shutdown();

    g_layout_api_initialized = false;
    ABE_Log(ABE_LOG_INFO, "LAYOUTAPI", "ABE Layout Engine shut down cleanly");
    return ABE_SUCCESS;
}

ABE_Error ABE_BuildRenderTree(ABE_DocumentHandle doc, ABE_RenderTreeHandle* out_tree) {
    return ABE_RenderTree_Build(doc, out_tree);
}

ABE_Error ABE_PerformLayout(ABE_RenderTreeHandle tree, float viewport_width, float viewport_height) {
    if (tree == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_RenderTree* rtree = ABE_RenderTree_Get(tree);
    if (!rtree || !rtree->root_node) return ABE_ERR_RENDER_TREE_FAILED;

    rtree->viewport_width = viewport_width;
    rtree->viewport_height = viewport_height;

    ABE_BlockLayout_Perform(rtree->root_node, viewport_width, viewport_height);
    ABE_Positioning_Apply(rtree->root_node, viewport_width, viewport_height);
    ABE_Overflow_Compute(rtree->root_node);

    ABE_Diag_RecordLayoutPerformed(25);
    return ABE_SUCCESS;
}

ABE_Error ABE_Reflow(ABE_RenderTreeHandle tree) {
    return ABE_Reflow_Perform(tree);
}

ABE_Error ABE_GetRenderTree(ABE_DocumentHandle doc, ABE_RenderTreeHandle* out_tree) {
    if (doc == ABE_INVALID_HANDLE || !out_tree) return ABE_ERR_INVALID_PARAM;
    return ABE_RenderTree_Build(doc, out_tree);
}

ABE_Error ABE_GetLayoutBox(ABE_RenderTreeHandle tree, ABE_NodeHandle node, ABE_LayoutBoxInfo* out_info) {
    if (tree == ABE_INVALID_HANDLE || node == ABE_INVALID_HANDLE || !out_info) return ABE_ERR_INVALID_PARAM;
    ABE_RenderTree* rtree = ABE_RenderTree_Get(tree);
    if (!rtree || !rtree->root_node) return ABE_ERR_RENDER_TREE_FAILED;

    ABE_RenderNode* rnode = ABE_RenderTree_FindNodeByDOMHandle(rtree->root_node, node);
    if (!rnode) return ABE_ERR_LAYOUT_BOX_NOT_FOUND;

    out_info->handle = rnode->handle;
    out_info->node_handle = rnode->dom_node_handle;
    out_info->content_box = rnode->content_box;
    out_info->margin = rnode->margin;
    out_info->padding = rnode->padding;
    out_info->border = rnode->border;
    out_info->overflow_box = rnode->overflow_box;
    out_info->is_anonymous = rnode->is_anonymous;
    out_info->is_inline = rnode->is_inline;
    out_info->is_flex = rnode->is_flex;
    out_info->is_dirty = rnode->is_dirty;
    out_info->visibility = rnode->style.visibility;
    return ABE_SUCCESS;
}

ABE_Error ABE_MarkDirty(ABE_RenderTreeHandle tree, ABE_NodeHandle node) {
    return ABE_Reflow_MarkNodeDirty(tree, node);
}

ABE_Error ABE_DestroyRenderTree(ABE_RenderTreeHandle tree) {
    return ABE_RenderTree_Destroy(tree);
}
