#include "surface.h"
#include "kernel/core/memory/heap/include/heap.h"
#include <stddef.h>

static uint32_t next_surface_id = 1;

struct BOSSurface* surface_create(int width, int height) {
    struct BOSSurface* surface = (struct BOSSurface*)kmalloc(sizeof(struct BOSSurface));
    if (!surface) return NULL;

    surface->unique_id = (uint64_t)next_surface_id++;
    surface->generation = 1;
    surface->owner_pid = 0; // Default to kernel/system, can be set later
    surface->x = 0;
    surface->y = 0;
    surface->width = width;
    surface->height = height;
    
    if (width > 0 && height > 0) {
        surface->framebuffer = (uint32_t*)kmalloc(width * height * sizeof(uint32_t));
        if (!surface->framebuffer) {
            kfree(surface);
            return NULL;
        }
        // Initial clear to black (transparent later if alpha supported)
        for (int i = 0; i < width * height; i++) {
            surface->framebuffer[i] = 0xFF000000;
        }
    } else {
        surface->framebuffer = NULL;
    }

    surface->visible = false;
    surface->dirty = true;
    surface->z_order = 0;
    
    surface->parent = NULL;
    surface->next = NULL;
    surface->children = NULL;

    return surface;
}

void surface_destroy(struct BOSSurface* surface) {
    if (!surface) return;

    // Recursively destroy children
    struct BOSSurface* current = surface->children;
    while (current) {
        struct BOSSurface* next = current->next;
        surface_destroy(current);
        current = next;
    }

    // Detach from parent
    if (surface->parent) {
        surface_remove_child(surface->parent, surface);
    }

    if (surface->framebuffer) {
        kfree(surface->framebuffer);
    }

    kfree(surface);
}

void surface_add_child(struct BOSSurface* parent, struct BOSSurface* child) {
    if (!parent || !child) return;
    
    // Detach from existing parent if necessary
    if (child->parent) {
        surface_remove_child(child->parent, child);
    }

    child->parent = parent;
    child->next = parent->children;
    parent->children = child;
}

void surface_remove_child(struct BOSSurface* parent, struct BOSSurface* child) {
    if (!parent || !child || child->parent != parent) return;

    struct BOSSurface** curr = &parent->children;
    while (*curr) {
        if (*curr == child) {
            *curr = child->next;
            child->parent = NULL;
            child->next = NULL;
            return;
        }
        curr = &(*curr)->next;
    }
}

void surface_set_position(struct BOSSurface* surface, int x, int y) {
    if (!surface) return;
    if (surface->x != x || surface->y != y) {
        surface->x = x;
        surface->y = y;
        surface_mark_dirty(surface);
    }
}

void surface_set_visible(struct BOSSurface* surface, bool visible) {
    if (!surface) return;
    if (surface->visible != visible) {
        surface->visible = visible;
        surface_mark_dirty(surface);
    }
}

void surface_mark_dirty(struct BOSSurface* surface) {
    if (!surface) return;
    surface->dirty = true;
}
