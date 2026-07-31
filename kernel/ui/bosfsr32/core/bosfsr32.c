#include "../include/bosfsr32.h"

extern void display_print(const char* s);

void bosfsr32_init(void) {
    display_print("[BOSFSR32] Framework Initialized Natively.\n");
}

void bosfsr32_render_window(BWE_Window* win, BOSFSR_Control* root) {
    if (!win || !root) return;
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb || !fb->buffer) return;

    BWE_Rect b = win->screen_bounds;
    int header = 28;
    int cy = b.y + header;

    bosfsr_paint_tree(root, fb, b.x, cy);
}
