#include "kernel/gui/controls/picturebox.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/render/painter.h"
#include <stddef.h>

static void picturebox_paint(BOSControl* self, struct BOSSurface* surface, const BVRect* clip) {
    BOSPictureBox* pb = (BOSPictureBox*)self;
    if (!self->visible) return;
    
    int ax, ay;
    control_get_absolute_position(self, &ax, &ay);
    
    if (pb->bitmap_data) {
        // Assume painter_draw_bitmap exists or stub it
        // painter_draw_bitmap(surface, ax, ay, pb->bitmap_data, pb->bmp_width, pb->bmp_height, clip);
    } else {
        BVRect rect = {ax, ay, self->width, self->height};
        BOVISUAL_Color bg = {0, 0, 0, 255};
        painter_fill_rect(surface, &rect, bg, clip);
    }
}

static void picturebox_destroy(BOSControl* self) {
    kfree(self);
}

BOSPictureBox* picturebox_create() {
    BOSPictureBox* pb = (BOSPictureBox*)kmalloc(sizeof(BOSPictureBox));
    if (!pb) return NULL;
    
    control_init(&pb->base);
    pb->base.paint = picturebox_paint;
    pb->base.handle_event = NULL; // No interaction
    pb->base.destroy = picturebox_destroy;
    
    pb->base.width = 100;
    pb->base.height = 100;
    pb->bitmap_data = NULL;
    pb->stretch = false;
    
    return pb;
}

void picturebox_set_image(BOSPictureBox* pb, uint32_t* data, int width, int height) {
    if (!pb) return;
    pb->bitmap_data = data;
    pb->bmp_width = width;
    pb->bmp_height = height;
    control_invalidate(&pb->base);
}
