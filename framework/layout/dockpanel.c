#include "framework/include/bos_ui_layout.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

BOS_DockPanel* BOS_DockPanel_Create(bool last_child_fill) {
    BOS_DockPanel* panel = (BOS_DockPanel*)kmalloc(sizeof(BOS_DockPanel));
    if (!panel) return NULL;

    memset(panel, 0, sizeof(BOS_DockPanel));
    BOS_UIElement_Init(&panel->base, "DockPanel");
    panel->last_child_fill = last_child_fill;
    return panel;
}

BOS_Canvas* BOS_Canvas_Create(void) {
    BOS_Canvas* canvas = (BOS_Canvas*)kmalloc(sizeof(BOS_Canvas));
    if (!canvas) return NULL;

    memset(canvas, 0, sizeof(BOS_Canvas));
    BOS_UIElement_Init(&canvas->base, "Canvas");
    return canvas;
}
