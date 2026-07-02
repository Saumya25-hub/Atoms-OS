#include "../../../wm/bwe/include/bwe.h"

static void bwe_canvas_render(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    extern bool BWE_GetClip(BWE_Rect* out_rect);

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    // Get current clipping rectangle
    BWE_Rect clip;
    if (!BWE_GetClip(&clip)) {
        clip.x = 0;
        clip.y = 0;
        clip.width = (int32_t)fb->width;
        clip.height = (int32_t)fb->height;
    }

    if (self->control_data.canvas.on_paint_canvas) {
        self->control_data.canvas.on_paint_canvas(self->id, fb, &clip);
    }
}

static void bwe_canvas_event(uint32_t window_id, const BWE_Event* event) {
    (void)window_id;
    (void)event;
}

bwe_error_t BOS_CreateCanvas(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void (*on_paint)(uint32_t, const BVFramebuffer*, const BWE_Rect*), uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_CANVAS;
        win->control_data.canvas.on_paint_canvas = on_paint;

        win->on_render = bwe_canvas_render;
        win->on_event = bwe_canvas_event;
    }

    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}
