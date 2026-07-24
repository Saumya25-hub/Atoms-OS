#include "abe_render_tree.h"
#include "../html/abe_html_document.h"
#include "../html/abe_dom_node.h"
#include "../css/abe_css_computed.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_RenderNodePool g_render_pool;
static ABE_RenderTree g_render_trees[ABE_MAX_RENDER_TREES];
static uint32_t g_next_render_id = 9000;
static bool g_render_tree_initialized = false;

static char ToLowerChar(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

static int StrCaseCmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return 1;
    while (*s1 && *s2) {
        char c1 = ToLowerChar(*s1);
        char c2 = ToLowerChar(*s2);
        if (c1 != c2) return 1;
        s1++; s2++;
    }
    return (*s1 == '\0' && *s2 == '\0') ? 0 : 1;
}

ABE_Error ABE_RenderTree_Init(void) {
    memset(&g_render_pool, 0, sizeof(ABE_RenderNodePool));
    memset(g_render_trees, 0, sizeof(g_render_trees));
    g_render_tree_initialized = true;
    ABE_Log(ABE_LOG_INFO, "RENDERTREE", "ABE Render Tree Builder Subsystem initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_RenderTree_Shutdown(void) {
    if (!g_render_tree_initialized) return ABE_ERR_NOT_INITIALIZED;
    for (uint32_t i = 0; i < ABE_MAX_RENDER_TREES; i++) {
        if (g_render_trees[i].in_use) {
            ABE_RenderTree_Destroy(g_render_trees[i].handle);
        }
    }
    g_render_tree_initialized = false;
    return ABE_SUCCESS;
}

ABE_RenderTree* ABE_RenderTree_Get(ABE_RenderTreeHandle handle) {
    if (!g_render_tree_initialized || handle == ABE_INVALID_HANDLE) return NULL;
    uint32_t slot = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_RENDER_TREES) return NULL;
    if (g_render_trees[slot].handle == handle && g_render_trees[slot].in_use) {
        return &g_render_trees[slot];
    }
    return NULL;
}

static ABE_RenderNode* AllocRenderNode(void) {
    for (uint32_t i = 0; i < ABE_MAX_RENDER_NODES; i++) {
        if (!g_render_pool.pool[i].in_use) {
            ABE_RenderNode* node = &g_render_pool.pool[i];
            memset(node, 0, sizeof(ABE_RenderNode));
            node->handle = (g_next_render_id++) | (i << 16);
            node->in_use = true;
            node->is_dirty = true;
            g_render_pool.active_count++;
            ABE_Diag_RecordRenderNodeAllocated();
            return node;
        }
    }
    return NULL;
}

static void FreeRenderNodeRecursive(ABE_RenderNode* node) {
    if (!node) return;

    ABE_RenderNode* child = node->first_child;
    while (child) {
        ABE_RenderNode* next = child->next_sibling;
        FreeRenderNodeRecursive(child);
        child = next;
    }

    node->in_use = false;
    if (g_render_pool.active_count > 0) g_render_pool.active_count--;
    ABE_Diag_RecordRenderNodeFreed();
}

static bool IsNonRenderableTag(const char* tag) {
    if (!tag) return false;
    if (StrCaseCmp(tag, "head") == 0) return true;
    if (StrCaseCmp(tag, "script") == 0) return true;
    if (StrCaseCmp(tag, "style") == 0) return true;
    if (StrCaseCmp(tag, "meta") == 0) return true;
    if (StrCaseCmp(tag, "title") == 0) return true;
    if (StrCaseCmp(tag, "link") == 0) return true;
    return false;
}

static ABE_RenderNode* BuildRenderNodeRecursive(ABE_DOMNode* dom_node) {
    if (!dom_node || !dom_node->in_use) return NULL;
    if (dom_node->type == ABE_NODE_COMMENT) return NULL;

    if (dom_node->type == ABE_NODE_ELEMENT && IsNonRenderableTag(dom_node->tag_name)) {
        return NULL;
    }

    ABE_ComputedStyle style;
    if (ABE_CSSComputed_GetNodeStyle(dom_node->handle, &style) == ABE_SUCCESS) {
        if (style.display == ABE_DISPLAY_NONE) return NULL;
    }

    ABE_RenderNode* rnode = AllocRenderNode();
    if (!rnode) return NULL;

    rnode->dom_node_handle = dom_node->handle;
    rnode->style = style;
    rnode->is_inline = (style.display == ABE_DISPLAY_INLINE || style.display == ABE_DISPLAY_INLINE_BLOCK);
    rnode->is_flex = (style.display == ABE_DISPLAY_FLEX);

    ABE_DOMNode* child_dom = dom_node->first_child;
    while (child_dom) {
        ABE_RenderNode* child_rnode = BuildRenderNodeRecursive(child_dom);
        if (child_rnode) {
            child_rnode->parent = rnode;
            if (!rnode->first_child) {
                rnode->first_child = child_rnode;
                rnode->last_child = child_rnode;
            } else {
                rnode->last_child->next_sibling = child_rnode;
                child_rnode->prev_sibling = rnode->last_child;
                rnode->last_child = child_rnode;
            }
        }
        child_dom = child_dom->next_sibling;
    }

    return rnode;
}

ABE_Error ABE_RenderTree_Build(ABE_DocumentHandle doc_handle, ABE_RenderTreeHandle* out_tree) {
    if (!g_render_tree_initialized || doc_handle == ABE_INVALID_HANDLE || !out_tree) return ABE_ERR_INVALID_PARAM;

    ABE_DocumentStruct* doc = ABE_HTMLDoc_Get(doc_handle);
    if (!doc || !doc->root_node) return ABE_ERR_INVALID_PARAM;

    uint32_t slot = ABE_INVALID_HANDLE;
    for (uint32_t i = 0; i < ABE_MAX_RENDER_TREES; i++) {
        if (!g_render_trees[i].in_use) {
            slot = i;
            break;
        }
    }
    if (slot == ABE_INVALID_HANDLE) return ABE_ERR_RESOURCE_EXHAUSTED;

    ABE_RenderTree* tree = &g_render_trees[slot];
    memset(tree, 0, sizeof(ABE_RenderTree));
    tree->handle = (g_next_render_id++) | (slot << 16);
    tree->doc_handle = doc_handle;
    tree->viewport_width = 1024.0f;
    tree->viewport_height = 768.0f;
    tree->in_use = true;

    tree->root_node = BuildRenderNodeRecursive(doc->root_node);
    if (!tree->root_node) {
        tree->in_use = false;
        return ABE_ERR_RENDER_TREE_FAILED;
    }

    *out_tree = tree->handle;
    ABE_Diag_RecordRenderTreeBuilt(g_render_pool.active_count);
    ABE_LogVal(ABE_LOG_INFO, "RENDERTREE", "Render Tree constructed cleanly, Node handle: ", tree->handle);
    return ABE_SUCCESS;
}

ABE_Error ABE_RenderTree_Destroy(ABE_RenderTreeHandle handle) {
    if (!g_render_tree_initialized || handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_RenderTree* tree = ABE_RenderTree_Get(handle);
    if (!tree) return ABE_ERR_INVALID_PARAM;

    if (tree->root_node) {
        FreeRenderNodeRecursive(tree->root_node);
        tree->root_node = NULL;
    }
    tree->in_use = false;
    ABE_LogVal(ABE_LOG_INFO, "RENDERTREE", "Destroyed Render Tree, Handle: ", handle);
    return ABE_SUCCESS;
}

ABE_RenderNode* ABE_RenderTree_FindNodeByDOMHandle(ABE_RenderNode* root, ABE_NodeHandle dom_handle) {
    if (!root || dom_handle == ABE_INVALID_HANDLE) return NULL;
    if (root->dom_node_handle == dom_handle) return root;

    ABE_RenderNode* child = root->first_child;
    while (child) {
        ABE_RenderNode* found = ABE_RenderTree_FindNodeByDOMHandle(child, dom_handle);
        if (found) return found;
        child = child->next_sibling;
    }
    return NULL;
}
