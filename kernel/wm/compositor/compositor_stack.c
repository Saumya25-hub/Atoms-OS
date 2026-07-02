#include "compositor_stack.h"
#include "compositor_surface.h"

static uint32_t g_focused_surface_id = 0;
static uint32_t g_max_z_order = 100;

void BOCompositorStack_Init(void) {
    g_focused_surface_id = 0;
    g_max_z_order = 100;
}

void BOCompositorStack_Insert(uint32_t id, uint32_t z_order) {
    BOCompositorSurface* surf = BOCompositorSurface_Find(id);
    if (surf) {
        surf->z_order = z_order;
        if (z_order > g_max_z_order) {
            g_max_z_order = z_order;
        }
    }
}

void BOCompositorStack_Remove(uint32_t id) {
    if (g_focused_surface_id == id) {
        g_focused_surface_id = 0;
    }
}

void BOCompositorStack_BringToFront(uint32_t id) {
    BOCompositorSurface* surf = BOCompositorSurface_Find(id);
    if (surf && id != 0) { // Never raise Desktop ID 0
        g_max_z_order++;
        surf->z_order = g_max_z_order;
        surf->is_dirty = true;
    }
}

void BOCompositorStack_SendToBack(uint32_t id) {
    BOCompositorSurface* surf = BOCompositorSurface_Find(id);
    if (surf && id != 0) {
        surf->z_order = 1; // Just above Desktop (0)
        surf->is_dirty = true;
    }
}

void BOCompositorStack_SetFocus(uint32_t id) {
    if (g_focused_surface_id != id) {
        BOCompositorSurface* old_focused = BOCompositorSurface_Find(g_focused_surface_id);
        if (old_focused) {
            old_focused->flags &= ~BOCOMPOSITOR_FLAG_FOCUSED;
            old_focused->is_dirty = true;
        }
        g_focused_surface_id = id;
        BOCompositorSurface* new_focused = BOCompositorSurface_Find(id);
        if (new_focused) {
            new_focused->flags |= BOCOMPOSITOR_FLAG_FOCUSED;
            new_focused->is_dirty = true;
        }
    }
}

uint32_t BOCompositorStack_GetFocusedID(void) {
    return g_focused_surface_id;
}

uint32_t BOCompositorStack_GetSortedVisible(uint32_t* out_ids, uint32_t max_count) {
    if (!out_ids || max_count == 0) return 0;

    uint32_t count = 0;
    for (uint32_t i = 0; i < BOCOMPOSITOR_MAX_SURFACES; i++) {
        BOCompositorSurface* surf = BOCompositorSurface_GetByIndex(i);
        if (surf && surf->active && (surf->flags & BOCOMPOSITOR_FLAG_VISIBLE) && surf->state == BOCOMPOSITOR_STATE_VISIBLE) {
            if (count < max_count) {
                out_ids[count++] = surf->id;
            }
        }
    }

    // Deterministic in-place insertion sort (Back-to-Front Z order)
    for (uint32_t i = 1; i < count; i++) {
        uint32_t key_id = out_ids[i];
        BOCompositorSurface* key_surf = BOCompositorSurface_Find(key_id);
        if (!key_surf) continue;

        int32_t j = (int32_t)i - 1;
        while (j >= 0) {
            BOCompositorSurface* prev_surf = BOCompositorSurface_Find(out_ids[j]);
            if (!prev_surf) break;

            // Sort condition: if prev has higher z_order, shift it up
            // If equal z_order, tie-break by ID ascending for determinism
            bool should_swap = false;
            if (prev_surf->z_order > key_surf->z_order) {
                should_swap = true;
            } else if (prev_surf->z_order == key_surf->z_order) {
                if (prev_surf->id > key_surf->id) {
                    should_swap = true;
                }
            }

            if (should_swap) {
                out_ids[j + 1] = out_ids[j];
                j--;
            } else {
                break;
            }
        }
        out_ids[j + 1] = key_id;
    }

    return count;
}
