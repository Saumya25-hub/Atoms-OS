#include "framework/include/bos_ui_controls.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

BOS_Window* BOS_Window_Create(int32_t x, int32_t y, uint32_t w, uint32_t h, const char* title) {
    BOS_Window* win = (BOS_Window*)kmalloc(sizeof(BOS_Window));
    if (!win) return NULL;

    memset(win, 0, sizeof(BOS_Window));
    BOS_UIElement_Init(&win->base, "Window");
    if (title) {
        strncpy(win->title, title, 127);
        win->title[127] = '\0';
    }

    /* Platform Window Handle Registration */
    BOS_WindowConfig cfg;
    cfg.x = x;
    cfg.y = y;
    cfg.width = w;
    cfg.height = h;
    cfg.title = win->title;
    cfg.flags = BOS_WINDOW_FLAG_NORMAL;

    BOS_CreateWindow(&cfg, &win->platform_handle);
    return win;
}

void BOS_Window_SetContent(BOS_Window* win, BOS_UIElement* content) {
    if (!win || !content) return;
    if (win->content) {
        BOS_UIElement_RemoveChild(&win->base, win->content);
    }
    win->content = content;
    BOS_UIElement_AddChild(&win->base, content);
}

void BOS_Window_Show(BOS_Window* win) {
    if (!win) return;
    BOS_ShowWindow(win->platform_handle);
}

BOS_Panel* BOS_Panel_Create(uint32_t bg_color) {
    BOS_Panel* panel = (BOS_Panel*)kmalloc(sizeof(BOS_Panel));
    if (!panel) return NULL;

    memset(panel, 0, sizeof(BOS_Panel));
    BOS_UIElement_Init(&panel->base, "Panel");
    panel->base.bg_color = bg_color;
    return panel;
}
