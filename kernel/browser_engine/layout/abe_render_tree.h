#ifndef ABE_RENDER_TREE_H
#define ABE_RENDER_TREE_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_RENDER_NODES 1024
#define ABE_MAX_RENDER_TREES 16

typedef struct ABE_RenderNode {
    ABE_LayoutBoxHandle handle;
    ABE_NodeHandle dom_node_handle;

    ABE_ComputedStyle style;

    ABE_Rect content_box;
    ABE_EdgeSizes margin;
    ABE_EdgeSizes padding;
    ABE_EdgeSizes border;
    ABE_Rect overflow_box;

    bool is_anonymous;
    bool is_inline;
    bool is_flex;
    bool is_dirty;

    struct ABE_RenderNode* parent;
    struct ABE_RenderNode* first_child;
    struct ABE_RenderNode* last_child;
    struct ABE_RenderNode* prev_sibling;
    struct ABE_RenderNode* next_sibling;

    bool in_use;
} ABE_RenderNode;

typedef struct {
    ABE_RenderTreeHandle handle;
    ABE_DocumentHandle doc_handle;
    ABE_RenderNode* root_node;
    uint32_t node_count;
    float viewport_width;
    float viewport_height;
    bool in_use;
} ABE_RenderTree;

typedef struct {
    ABE_RenderNode pool[ABE_MAX_RENDER_NODES];
    uint32_t active_count;
} ABE_RenderNodePool;

ABE_Error ABE_RenderTree_Init(void);
ABE_Error ABE_RenderTree_Shutdown(void);

ABE_Error ABE_RenderTree_Build(ABE_DocumentHandle doc_handle, ABE_RenderTreeHandle* out_tree);
ABE_Error ABE_RenderTree_Destroy(ABE_RenderTreeHandle handle);

ABE_RenderTree* ABE_RenderTree_Get(ABE_RenderTreeHandle handle);
ABE_RenderNode* ABE_RenderTree_FindNodeByDOMHandle(ABE_RenderNode* root, ABE_NodeHandle dom_handle);

#ifdef __cplusplus
}
#endif

#endif // ABE_RENDER_TREE_H
