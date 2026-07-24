#ifndef ATRIX_RENDER_TREE_H
#define ATRIX_RENDER_TREE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX Render Tree & Painting Pipeline Subsystem
// ============================================================

typedef struct {
    int32_t  x;
    int32_t  y;
    int32_t  width;
    int32_t  height;
    uint32_t bg_color_argb;
    uint32_t text_color_argb;
    char     text[128];
} RenderObject;

typedef struct {
    RenderObject objects[64];
    uint32_t     object_count;
} RenderTree;

void        ATRIX_RenderTree_Init(void);
RenderTree* ATRIX_RenderTree_Build(void);
void        ATRIX_RenderTree_Paint(const RenderTree* tree, void* fb_target);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_RENDER_TREE_H
