#include "compositor.h"
#include "kernel/gui/render/painter.h"
#include "kernel/drivers/video/vbe/vbe.h" // For framebuffer flip

static struct BOSSurface* desktop_root = NULL;
static DirtyRegion global_dirty_region;
static int drawn_rects = 0;
static int composition_time_ms = 0; // Simplified for now

void compositor_init(struct BOSSurface* root_surface) {
    desktop_root = root_surface;
    dirty_region_init(&global_dirty_region);
    
    // Invalidate full screen initially
    if (root_surface) {
        BVRect full_screen = {0, 0, root_surface->width, root_surface->height};
        compositor_invalidate_rect(&full_screen);
    }
}

void compositor_invalidate_rect(const BVRect* rect) {
    dirty_region_add(&global_dirty_region, rect);
}

void compositor_invalidate_surface(struct BOSSurface* surface) {
    if (!surface) return;
    BVRect rect = {surface->x, surface->y, surface->width, surface->height};
    compositor_invalidate_rect(&rect);
    surface->dirty = true;
}

// Helper to draw a surface and its children recursively in z-order
static void compose_tree(struct BOSSurface* surface, struct BOSSurface* target, const BVRect* clip) {
    if (!surface || !surface->visible) return;

    // Determine if this surface intersects with the clip rect
    BVRect surface_rect = {surface->x, surface->y, surface->width, surface->height};
    BVRect intersection;
    if (dirty_region_clip(&intersection, &surface_rect, clip)) {
        // Draw this surface onto the target (which is usually the desktop/backbuffer)
        painter_draw_surface(target, surface, surface->x, surface->y, clip);
        drawn_rects++;
    }

    // Sort children by z-order (omitted for brevity, assume they are inserted in order for now)
    // Draw children
    struct BOSSurface* child = surface->children;
    while (child) {
        compose_tree(child, target, clip);
        child = child->next;
    }
}

void compositor_compose(void) {
    if (!desktop_root) return;

    // Start timing (pseudo)
    drawn_rects = 0;

    // Get back buffer from VBE (assuming it gives us a BVFramebuffer)
    extern BVFramebuffer* vbe_get_back_page_ptr(void);
    BVFramebuffer* back_fb_ptr = vbe_get_back_page_ptr();
    
    // If VBE provides the buffer directly to our root surface, 
    // we assume desktop_root->framebuffer is mapped to back_fb.buffer.
    if (back_fb_ptr->buffer != NULL) {
        // Ensure root surface wraps the back buffer for the painter
        desktop_root->framebuffer = (uint32_t*)back_fb_ptr->buffer;
        desktop_root->width = back_fb_ptr->width;
        desktop_root->height = back_fb_ptr->height;
    }

    // Process each dirty rectangle
    for (int i = 0; i < global_dirty_region.count; i++) {
        BVRect* clip = &global_dirty_region.rects[i];
        
        // Clear background (optional if desktop covers it)
        painter_fill_rect(desktop_root, clip, 0xFF000000, clip);

        // Render desktop wallpaper/color
        // ... (handled by compose_tree if desktop draws itself, or draw it here)
        
        // Draw window tree back-to-front
        struct BOSSurface* child = desktop_root->children;
        while (child) {
            compose_tree(child, desktop_root, clip);
            child = child->next;
        }
    }

        // Swap front and back buffer (only if AGDTE is not managing presentation)
        extern bool AGDTE_IsInitialized(void);
        if (!AGDTE_IsInitialized()) {
            vbe_swap_page();
        }
        dirty_region_clear(&global_dirty_region);
    }
    
    // End timing
    // composition_time_ms = ...
}

void compositor_force_redraw(void) {
    if (desktop_root) {
        BVRect full = {0, 0, desktop_root->width, desktop_root->height};
        compositor_invalidate_rect(&full);
    }
}

int compositor_get_drawn_rects(void) {
    return drawn_rects;
}

int compositor_get_composition_time_ms(void) {
    return composition_time_ms;
}
