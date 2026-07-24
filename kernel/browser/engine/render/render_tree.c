#include "render_tree.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ATRIX_RenderTree_Init(void) {
    bwe_log("INFO", "ATRIX Render Tree & Painting Pipeline Subsystem Initialized");
}

RenderTree* ATRIX_RenderTree_Build(void) {
    RenderTree* tree = (RenderTree*)kmalloc(sizeof(RenderTree));
    if (!tree) return 0;

    tree->object_count = 1;
    tree->objects[0].x = 10;
    tree->objects[0].y = 10;
    tree->objects[0].width = 600;
    tree->objects[0].height = 400;
    tree->objects[0].bg_color_argb = 0xFF1E1E2E;
    tree->objects[0].text_color_argb = 0xFFCAD3F5;
    memcpy(tree->objects[0].text, "ATRIX Engine Rendered Page", 26);

    return tree;
}

void ATRIX_RenderTree_Paint(const RenderTree* tree, void* fb_target) {
    // Pipeline paint dispatch
}
