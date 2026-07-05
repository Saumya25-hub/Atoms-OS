#ifndef GUI_SURFACE_H
#define GUI_SURFACE_H

#include <stdint.h>
#include <stdbool.h>

struct BOSSurface {
    uint64_t unique_id;
    uint32_t generation;
    uint32_t owner_pid;
    int x;
    int y;
    int width;
    int height;
    uint32_t* framebuffer;
    bool visible;
    bool dirty;
    uint32_t z_order;
    struct BOSSurface* parent;
    struct BOSSurface* next;
    struct BOSSurface* children;
};

// Allocation and Lifecycle
struct BOSSurface* surface_create(int width, int height);
void surface_destroy(struct BOSSurface* surface);

// Hierarchy Management
void surface_add_child(struct BOSSurface* parent, struct BOSSurface* child);
void surface_remove_child(struct BOSSurface* parent, struct BOSSurface* child);

// Property Updates
void surface_set_position(struct BOSSurface* surface, int x, int y);
void surface_set_visible(struct BOSSurface* surface, bool visible);
void surface_mark_dirty(struct BOSSurface* surface);

#endif // GUI_SURFACE_H
