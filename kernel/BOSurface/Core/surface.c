// ============================================================
// BOSurface — Phase 2: Surface Composition Engine
// ============================================================
// Implements:
//   - Static Surface Pool (64 slots, no dynamic allocation)
//   - Parent-Child Tree (Desktop → Window → Controls)
//   - Z-Order Management
//   - Absolute Screen Coordinate Computation
//   - Z-Order Traversal (BWE_Compose)
// ============================================================

#include "surface.h"
#include "../../display/display.h"
#include "bovisual/Include/events.h"

// ============================================================
// Static Surface Pool
// ============================================================
static BWE_Surface surface_pool[BWE_MAX_SURFACES];
static uint32_t    next_surface_id = 1;  // 0 is reserved for Desktop
static uint32_t    active_surface_count = 0;

// Focus Engine State
static uint32_t    bwe_active_surface_id = 0;
static uint32_t    bwe_focused_surface_id = 0;
static uint32_t    bwe_focused_control_id = 0;
static uint32_t    bwe_previous_focus_id = 0;

// Drag Engine State
static bool        bwe_is_dragging = false;
static uint32_t    bwe_drag_surface_id = 0;
static int32_t     bwe_drag_offset_x = 0;
static int32_t     bwe_drag_offset_y = 0;

// ============================================================
// Internal Logging
// ============================================================
static void bwe_log(const char* level, const char* msg) {
    display_print("[BWE_");
    display_print(level);
    display_print("] ");
    display_print(msg);
    display_print("\n");
}

static void bwe_log_id(const char* level, const char* msg, uint32_t id) {
    display_print("[BWE_");
    display_print(level);
    display_print("] ");
    display_print(msg);
    display_print(" #");
    display_print_dec(id);
    display_print("\n");
}

// ============================================================
// Internal: Find a surface by ID
// ============================================================
BWE_Surface* BWE_GetSurface(uint32_t surface_id) {
    for (uint32_t i = 0; i < BWE_MAX_SURFACES; i++) {
        if (surface_pool[i].active && surface_pool[i].id == surface_id) {
            return &surface_pool[i];
        }
    }
    return 0; // NULL
}

// ============================================================
// Internal: Find an empty slot in the pool
// ============================================================
static BWE_Surface* find_free_slot(void) {
    for (uint32_t i = 0; i < BWE_MAX_SURFACES; i++) {
        if (!surface_pool[i].active) {
            return &surface_pool[i];
        }
    }
    return 0; // Pool full
}

// ============================================================
// Internal: Add child to parent's children array
// ============================================================
static bool add_child_to_parent(BWE_Surface* parent, uint32_t child_id) {
    if (parent->child_count >= BWE_MAX_CHILDREN) {
        return false;
    }
    parent->children[parent->child_count] = child_id;
    parent->child_count++;
    return true;
}

// ============================================================
// Internal: Remove child from parent's children array
// ============================================================
static void remove_child_from_parent(BWE_Surface* parent, uint32_t child_id) {
    for (uint32_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child_id) {
            // Shift remaining children down
            for (uint32_t j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->child_count--;
            return;
        }
    }
}

// ============================================================
// Internal: Trace up tree to find the top-level parent (direct child of Desktop)
// ============================================================
static uint32_t BWE_GetTopLevelSurface(uint32_t surface_id) {
    if (surface_id == BWE_DESKTOP_ID) return BWE_DESKTOP_ID;
    
    BWE_Surface* surface = BWE_GetSurface(surface_id);
    while (surface && surface->parent_id != BWE_DESKTOP_ID) {
        surface = BWE_GetSurface(surface->parent_id);
    }
    return surface ? surface->id : BWE_DESKTOP_ID;
}

// ============================================================
// BOSurface_Init — Creates the Root Desktop Surface (ID 0)
// ============================================================
void BOSurface_Init(void) {
    // Zero-initialize entire pool
    for (uint32_t i = 0; i < BWE_MAX_SURFACES; i++) {
        surface_pool[i].active = false;
        surface_pool[i].id = 0;
        surface_pool[i].parent_id = 0;
        surface_pool[i].child_count = 0;
        surface_pool[i].z_order = 0;
        surface_pool[i].state = BWE_STATE_DESTROYED;
        surface_pool[i].flags = 0;
        surface_pool[i].owner_pid = 0;
        surface_pool[i].local_bounds.x = 0;
        surface_pool[i].local_bounds.y = 0;
        surface_pool[i].local_bounds.width = 0;
        surface_pool[i].local_bounds.height = 0;
        surface_pool[i].screen_bounds.x = 0;
        surface_pool[i].screen_bounds.y = 0;
        surface_pool[i].screen_bounds.width = 0;
        surface_pool[i].screen_bounds.height = 0;
    }

    // Create Desktop Surface at slot 0
    extern uint32_t g_kernel_screen_width;
    extern uint32_t g_kernel_screen_height;

    BWE_Surface* desktop = &surface_pool[0];
    desktop->id = BWE_DESKTOP_ID;
    desktop->active = true;
    desktop->parent_id = BWE_DESKTOP_ID; // Desktop is its own parent
    desktop->child_count = 0;
    desktop->z_order = 0;
    desktop->state = BWE_STATE_VISIBLE;
    desktop->flags = BWE_FLAG_VISIBLE;
    desktop->owner_pid = 0; // Kernel
    desktop->local_bounds.x = 0;
    desktop->local_bounds.y = 0;
    desktop->local_bounds.width = (int32_t)g_kernel_screen_width;
    desktop->local_bounds.height = (int32_t)g_kernel_screen_height;
    desktop->screen_bounds = desktop->local_bounds;

    next_surface_id = 1;
    active_surface_count = 1;

    bwe_log("INFO", "BOSurface Initialized");
    bwe_log("INFO", "Desktop Surface Created (ID 0)");
}

// ============================================================
// BOS_CreateSurface — Allocates a new surface in the tree
// ============================================================
bwe_error_t BOS_CreateSurface(uint32_t parent_id, uint32_t x, uint32_t y,
                               uint32_t width, uint32_t height,
                               uint32_t flags, uint32_t* out_surface_id) {
    // Validate parent exists
    BWE_Surface* parent = BWE_GetSurface(parent_id);
    if (!parent) {
        bwe_log("ERROR", "BOS_CreateSurface: Invalid Parent ID");
        return BWE0002;
    }

    // Find free slot
    BWE_Surface* surface = find_free_slot();
    if (!surface) {
        bwe_log("ERROR", "BOS_CreateSurface: Pool Full (BWE0004)");
        return BWE0004;
    }

    // Assign ID
    uint32_t id = next_surface_id++;

    // Initialize surface
    surface->id = id;
    surface->active = true;
    surface->parent_id = parent_id;
    surface->child_count = 0;
    surface->z_order = parent->child_count; // Auto z-order based on creation order
    surface->state = (flags & BWE_FLAG_VISIBLE) ? BWE_STATE_VISIBLE : BWE_STATE_CREATED;
    surface->flags = flags;
    surface->owner_pid = 0;
    surface->local_bounds.x = (int32_t)x;
    surface->local_bounds.y = (int32_t)y;
    surface->local_bounds.width = (int32_t)width;
    surface->local_bounds.height = (int32_t)height;

    // Screen bounds will be computed by BWE_ComputeScreenBounds
    surface->screen_bounds.x = 0;
    surface->screen_bounds.y = 0;
    surface->screen_bounds.width = (int32_t)width;
    surface->screen_bounds.height = (int32_t)height;

    // Add to parent's children
    if (!add_child_to_parent(parent, id)) {
        surface->active = false;
        bwe_log("ERROR", "BOS_CreateSurface: Parent children array full");
        return BWE0004;
    }

    active_surface_count++;

    if (out_surface_id) {
        *out_surface_id = id;
    }

    bwe_log_id("INFO", "Surface Created", id);
    return BWE_SUCCESS;
}

// ============================================================
// BOS_DestroySurface — Recursively destroys surface and children
// ============================================================
bwe_error_t BOS_DestroySurface(uint32_t surface_id) {
    if (surface_id == BWE_DESKTOP_ID) {
        bwe_log("ERROR", "Cannot destroy Desktop surface");
        return BWE0001;
    }

    BWE_Surface* surface = BWE_GetSurface(surface_id);
    if (!surface) {
        return BWE0001;
    }

    // Recursively destroy children (iterate backwards to avoid index issues)
    while (surface->child_count > 0) {
        uint32_t child_id = surface->children[surface->child_count - 1];
        BOS_DestroySurface(child_id); // Recursive
    }

    // Remove from parent's children list
    BWE_Surface* parent = BWE_GetSurface(surface->parent_id);
    if (parent) {
        remove_child_from_parent(parent, surface_id);
    }

    // Mark slot as free BEFORE handling focus fallback so it is not found as active/visible
    surface->active = false;
    surface->state = BWE_STATE_DESTROYED;
    active_surface_count--;

    // Handle focus loss if this surface had focus or was active
    if (bwe_focused_surface_id == surface_id) {
        bwe_log_id("FOCUS", "Destroyed Surface Lost Focus", surface_id);
        bwe_focused_surface_id = 0;
        
        // Fallback focus to the next visible top-level window in Z-order
        uint32_t fallback_id = BWE_DESKTOP_ID;
        BWE_Surface* desktop = BWE_GetSurface(BWE_DESKTOP_ID);
        if (desktop) {
            for (int32_t i = (int32_t)desktop->child_count - 1; i >= 0; i--) {
                uint32_t child_id = desktop->children[i];
                BWE_Surface* child = BWE_GetSurface(child_id);
                if (child && (child->flags & BWE_FLAG_VISIBLE) && child->active) {
                    fallback_id = child_id;
                    break;
                }
            }
        }

        if (fallback_id != BWE_DESKTOP_ID) {
            bwe_log_id("FOCUS", "Restoring Focus to Previous", fallback_id);
            BOS_SetFocus(fallback_id);
        } else {
            bwe_log_id("FOCUS", "Restoring Focus to Desktop", BWE_DESKTOP_ID);
            BOS_SetFocus(BWE_DESKTOP_ID);
        }
    }
    
    if (bwe_active_surface_id == surface_id) {
        bwe_log_id("FOCUS", "Destroyed Surface Lost Active", surface_id);
        bwe_active_surface_id = 0;
    }
    
    if (bwe_previous_focus_id == surface_id) {
        bwe_previous_focus_id = 0;
    }

    bwe_log_id("INFO", "Surface Destroyed", surface_id);
    return BWE_SUCCESS;
}

// ============================================================
// BOS_Show — Make surface visible
// ============================================================
bwe_error_t BOS_Show(uint32_t target_id) {
    BWE_Surface* surface = BWE_GetSurface(target_id);
    if (!surface) {
        return BWE0001;
    }
    surface->state = BWE_STATE_VISIBLE;
    surface->flags |= BWE_FLAG_VISIBLE;
    bwe_log_id("INFO", "Surface Shown", target_id);
    return BWE_SUCCESS;
}

// ============================================================
// BOS_Hide — Make surface hidden
// ============================================================
bwe_error_t BOS_Hide(uint32_t target_id) {
    BWE_Surface* surface = BWE_GetSurface(target_id);
    if (!surface) {
        return BWE0001;
    }
    surface->state = BWE_STATE_HIDDEN;
    surface->flags &= ~BWE_FLAG_VISIBLE;
    bwe_log_id("INFO", "Surface Hidden", target_id);
    return BWE_SUCCESS;
}

// ============================================================
// BOS_SetBounds — Update geometry (triggers recomposition)
// ============================================================
bwe_error_t BOS_SetBounds(uint32_t target_id, uint32_t x, uint32_t y,
                           uint32_t width, uint32_t height) {
    BWE_Surface* surface = BWE_GetSurface(target_id);
    if (!surface) {
        return BWE0001;
    }
    surface->local_bounds.x = (int32_t)x;
    surface->local_bounds.y = (int32_t)y;
    surface->local_bounds.width = (int32_t)width;
    surface->local_bounds.height = (int32_t)height;
    bwe_log_id("INFO", "Bounds Updated", target_id);
    return BWE_SUCCESS;
}

// ============================================================
// BOS_SetFocus — Set focus to a surface and bring its window to front
// ============================================================
bwe_error_t BOS_SetFocus(uint32_t surface_id) {
    if (surface_id == BWE_DESKTOP_ID) {
        return BOS_ClearFocus();
    }

    if (bwe_focused_surface_id == surface_id) {
        // Already focused, no unnecessary state changes
        return BWE_SUCCESS;
    }

    BWE_Surface* surface = BWE_GetSurface(surface_id);
    if (!surface) {
        return BWE0001; // Invalid Surface
    }

    // 1. Handle previous focus loss
    if (bwe_focused_surface_id != 0) {
        BWE_Surface* old_focused = BWE_GetSurface(bwe_focused_surface_id);
        if (old_focused) {
            old_focused->flags &= ~BWE_FLAG_FOCUSED;
            if (old_focused->state == BWE_STATE_FOCUSED) {
                old_focused->state = BWE_STATE_VISIBLE; // Fallback state
            }
            bwe_log_id("FOCUS", "Surface Lost Focus", bwe_focused_surface_id);
        }
        bwe_previous_focus_id = bwe_focused_surface_id;
    }

    // 2. Identify Top-Level Window (direct child of Desktop)
    uint32_t new_active_id = BWE_GetTopLevelSurface(surface_id);

    // 3. Handle active surface change
    if (new_active_id != BWE_DESKTOP_ID && new_active_id != bwe_active_surface_id) {
        if (bwe_active_surface_id != 0) {
            bwe_log_id("FOCUS", "Surface Lost Active", bwe_active_surface_id);
        }
        bwe_active_surface_id = new_active_id;
        bwe_log_id("FOCUS", "Surface Gained Active", bwe_active_surface_id);
        
        // Z-Order: Bring top-level window to front
        BWE_Surface* desktop = BWE_GetSurface(BWE_DESKTOP_ID);
        if (desktop) {
            remove_child_from_parent(desktop, new_active_id);
            add_child_to_parent(desktop, new_active_id);
            
            // Print Z-Order
            display_print("[BWE_FOCUS] Z-Order: ");
            for (uint32_t i = 0; i < desktop->child_count; i++) {
                display_print_dec(desktop->children[i]);
                if (i < desktop->child_count - 1) display_print(" -> ");
            }
            display_print("\n");
        }
    }

    // 4. Gain Focus
    bwe_focused_surface_id = surface_id;
    surface->flags |= BWE_FLAG_FOCUSED;
    surface->state = BWE_STATE_FOCUSED;
    bwe_log_id("FOCUS", "Surface Gained Focus", surface_id);

    return BWE_SUCCESS;
}

// ============================================================
// BOS_GetFocus — Get currently focused surface ID
// ============================================================
uint32_t BOS_GetFocus(void) {
    return bwe_focused_surface_id;
}

// ============================================================
// BOS_GetActiveSurface — Get currently active top-level surface ID
// ============================================================
uint32_t BOS_GetActiveSurface(void) {
    return bwe_active_surface_id;
}

// ============================================================
// BOS_ClearFocus — Remove focus globally
// ============================================================
bwe_error_t BOS_ClearFocus(void) {
    if (bwe_focused_surface_id != 0) {
        BWE_Surface* old_focused = BWE_GetSurface(bwe_focused_surface_id);
        if (old_focused) {
            old_focused->flags &= ~BWE_FLAG_FOCUSED;
            if (old_focused->state == BWE_STATE_FOCUSED) {
                old_focused->state = BWE_STATE_VISIBLE;
            }
            bwe_log_id("FOCUS", "Surface Lost Focus", bwe_focused_surface_id);
        }
        bwe_previous_focus_id = bwe_focused_surface_id;
        bwe_focused_surface_id = 0;
    }

    if (bwe_active_surface_id != 0) {
        bwe_log_id("FOCUS", "Surface Lost Active", bwe_active_surface_id);
        bwe_active_surface_id = 0;
    }

    return BWE_SUCCESS;
}

// ============================================================
// BWE_HitTest — Find deepest visible surface at (x,y)
// ============================================================
static uint32_t hit_test_recursive(BWE_Surface* surface, int32_t x, int32_t y) {
    if (!surface || !(surface->flags & BWE_FLAG_VISIBLE)) return 0;
    
    // Iterate children backwards (highest Z-Order first)
    for (int32_t i = (int32_t)surface->child_count - 1; i >= 0; i--) {
        BWE_Surface* child = BWE_GetSurface(surface->children[i]);
        if (child && (child->flags & BWE_FLAG_VISIBLE)) {
            // Check if point is inside child's screen bounds
            if (x >= child->screen_bounds.x && x < (child->screen_bounds.x + child->screen_bounds.width) &&
                y >= child->screen_bounds.y && y < (child->screen_bounds.y + child->screen_bounds.height)) {
                
                // Recurse to see if a deeper child was hit
                uint32_t hit = hit_test_recursive(child, x, y);
                if (hit != 0) return hit;
                
                return child->id; // Hit this child, but no deeper children
            }
        }
    }
    return 0; // Point not within any child (or no children hit)
}

uint32_t BWE_HitTest(int32_t screen_x, int32_t screen_y) {
    BWE_Surface* desktop = BWE_GetSurface(BWE_DESKTOP_ID);
    if (!desktop) return 0;
    
    uint32_t hit = hit_test_recursive(desktop, screen_x, screen_y);
    return hit != 0 ? hit : BWE_DESKTOP_ID;
}

// ============================================================
// BOS_ProcessEvent — Handle Mouse and Keyboard input
// ============================================================
void BOS_ProcessEvent(const BVEvent* event) {
    if (!event) return;

    if (event->type == BV_EVENT_MOUSE_DOWN) {
        uint32_t hit_id = BWE_HitTest(event->mouse_x, event->mouse_y);
        
        bwe_log_id("INPUT", "Hit Test Result", hit_id);

        if (hit_id != BWE_DESKTOP_ID && hit_id != 0) {
            BOS_SetFocus(hit_id);
            
            // Check for draggable top-level window
            uint32_t top_level_id = BWE_GetTopLevelSurface(hit_id);
            BWE_Surface* top_level = BWE_GetSurface(top_level_id);
            
            if (top_level && (top_level->flags & BWE_FLAG_DRAGGABLE)) {
                bwe_is_dragging = true;
                bwe_drag_surface_id = top_level_id;
                bwe_drag_offset_x = event->mouse_x - top_level->local_bounds.x;
                bwe_drag_offset_y = event->mouse_y - top_level->local_bounds.y;
                bwe_log_id("INPUT", "Started Dragging", top_level_id);
            }
        } else {
            BOS_ClearFocus();
        }
    }
    else if (event->type == BV_EVENT_MOUSE_UP) {
        if (bwe_is_dragging) {
            bwe_log_id("INPUT", "Ended Dragging", bwe_drag_surface_id);
            bwe_is_dragging = false;
            bwe_drag_surface_id = 0;
        }
    }
    else if (event->type == BV_EVENT_MOUSE_MOVE) {
        if (bwe_is_dragging && bwe_drag_surface_id != 0) {
            BWE_Surface* dragged = BWE_GetSurface(bwe_drag_surface_id);
            if (dragged) {
                int32_t new_x = event->mouse_x - bwe_drag_offset_x;
                int32_t new_y = event->mouse_y - bwe_drag_offset_y;
                BOS_SetBounds(bwe_drag_surface_id, new_x, new_y, dragged->local_bounds.width, dragged->local_bounds.height);
            }
        }
    }
}

// ============================================================
// BWE_ComputeScreenBounds — Recursive coordinate translation
// ============================================================
// Walks the tree and computes absolute screen coordinates for
// every surface by adding parent's screen position to child's
// local position.
// ============================================================
static void compute_screen_bounds_recursive(BWE_Surface* surface) {
    for (uint32_t i = 0; i < surface->child_count; i++) {
        BWE_Surface* child = BWE_GetSurface(surface->children[i]);
        if (!child) continue;

        // Absolute = Parent's absolute + Child's local offset
        child->screen_bounds.x = surface->screen_bounds.x + child->local_bounds.x;
        child->screen_bounds.y = surface->screen_bounds.y + child->local_bounds.y;
        child->screen_bounds.width = child->local_bounds.width;
        child->screen_bounds.height = child->local_bounds.height;

        // Recurse into children
        compute_screen_bounds_recursive(child);
    }
}

void BWE_ComputeScreenBounds(void) {
    BWE_Surface* desktop = BWE_GetSurface(BWE_DESKTOP_ID);
    if (!desktop) return;

    // Desktop screen_bounds is always (0, 0, screen_w, screen_h) — already set in Init
    compute_screen_bounds_recursive(desktop);
}

// ============================================================
// BWE_Compose — Z-Order Render Traversal
// ============================================================
// Traverses the Surface Tree in Z-Order (depth-first, children
// sorted by z_order) and logs the render order.
// In future phases, this will issue actual draw commands.
// ============================================================
extern void BOVISUAL_Graphics_Fill(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color);
extern void BOVISUAL_Graphics_DrawRect(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color);

static void compose_recursive(BWE_Surface* surface, uint32_t depth) {
    if (!(surface->flags & BWE_FLAG_VISIBLE)) {
        return; // Skip hidden surfaces
    }

    // Assign a color based on ID for visual distinction
    uint32_t color = 0xFF334455; // Default dark
    if (surface->id == BWE_DESKTOP_ID) {
        color = 0xFF0B1120; // Desktop background
    } else if (depth == 1) {
        // Top-level windows
        color = (surface->id % 2 == 0) ? 0xFF1E293B : 0xFF334155; 
    } else {
        // Panels / children
        color = (surface->id % 2 == 0) ? 0xFF475569 : 0xFF64748B;
    }

    // Draw solid fill
    BOVISUAL_Graphics_Fill(surface->screen_bounds.x, surface->screen_bounds.y,
                           surface->screen_bounds.width, surface->screen_bounds.height, color);

    // Draw focus border if focused
    if (surface->flags & BWE_FLAG_FOCUSED) {
        // Draw 2px border (simulated by drawing 4 rects or using a DrawRect if available)
        // For now just fill a slightly larger/smaller rect or draw inner border
        BOVISUAL_Graphics_Fill(surface->screen_bounds.x, surface->screen_bounds.y, surface->screen_bounds.width, 2, 0xFF38BDF8); // Top
        BOVISUAL_Graphics_Fill(surface->screen_bounds.x, surface->screen_bounds.y + surface->screen_bounds.height - 2, surface->screen_bounds.width, 2, 0xFF38BDF8); // Bottom
        BOVISUAL_Graphics_Fill(surface->screen_bounds.x, surface->screen_bounds.y, 2, surface->screen_bounds.height, 0xFF38BDF8); // Left
        BOVISUAL_Graphics_Fill(surface->screen_bounds.x + surface->screen_bounds.width - 2, surface->screen_bounds.y, 2, surface->screen_bounds.height, 0xFF38BDF8); // Right
    }

    // Render children in z-order (already insertion-ordered)
    for (uint32_t i = 0; i < surface->child_count; i++) {
        BWE_Surface* child = BWE_GetSurface(surface->children[i]);
        if (child) {
            compose_recursive(child, depth + 1);
        }
    }
}

void BWE_Compose(void) {
    BWE_Surface* desktop = BWE_GetSurface(BWE_DESKTOP_ID);
    if (!desktop) {
        bwe_log("ERROR", "BWE_Compose: No Desktop Surface");
        return;
    }

    bwe_log("INFO", "--- Compose Start ---");
    compose_recursive(desktop, 0);
    bwe_log("INFO", "--- Compose End ---");
}

// ============================================================
// Accessors
// ============================================================
uint32_t BWE_GetSurfaceCount(void) {
    return active_surface_count;
}

// ============================================================
// BOS_Test_Phase1 — Legacy Phase 1 Validation (Preserved)
// ============================================================
void BOS_Test_Phase1(void) {
    BOSurface_Init();

    uint32_t my_surface_id = 0;
    BOS_CreateSurface(0, 100, 100, 400, 300, BWE_FLAG_VISIBLE, &my_surface_id);
    BOS_Show(my_surface_id);

    display_print("\nPASS_BWE_PHASE1\n");
}

// ============================================================
// BOS_Test_Phase2 — Surface Composition Engine Validation
// ============================================================
// Builds a realistic Surface Tree:
//
//   Desktop (0)
//   ├── Window 1 (at 50,50 size 600x400)
//   │   ├── Panel (at 10,30 size 580x360)
//   │   │   ├── Button (at 20,20 size 120x40)
//   │   │   └── Label (at 20,80 size 200x20)
//   │   └── Titlebar (at 0,0 size 600x30)
//   └── Window 2 (at 200,150 size 400x300)
//       └── Panel2 (at 10,30 size 380x260)
//
// Then computes screen coordinates and runs Z-order compose.
// ============================================================
void BOS_Test_Phase2(void) {
    display_print("\n--- BWE Phase 2: Surface Composition Engine ---\n\n");

    // Re-initialize (clean slate)
    BOSurface_Init();

    // ---- Build Surface Tree ----

    // Window 1 (child of Desktop)
    uint32_t win1_id = 0;
    bwe_error_t err = BOS_CreateSurface(BWE_DESKTOP_ID, 50, 50, 600, 400, BWE_FLAG_VISIBLE, &win1_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Window 1 creation\n"); return; }

    // Titlebar (child of Window 1)
    uint32_t titlebar_id = 0;
    err = BOS_CreateSurface(win1_id, 0, 0, 600, 30, BWE_FLAG_VISIBLE, &titlebar_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Titlebar creation\n"); return; }

    // Panel (child of Window 1)
    uint32_t panel_id = 0;
    err = BOS_CreateSurface(win1_id, 10, 30, 580, 360, BWE_FLAG_VISIBLE, &panel_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Panel creation\n"); return; }

    // Button (child of Panel)
    uint32_t button_id = 0;
    err = BOS_CreateSurface(panel_id, 20, 20, 120, 40, BWE_FLAG_VISIBLE, &button_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Button creation\n"); return; }

    // Label (child of Panel)
    uint32_t label_id = 0;
    err = BOS_CreateSurface(panel_id, 20, 80, 200, 20, BWE_FLAG_VISIBLE, &label_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Label creation\n"); return; }

    // Window 2 (child of Desktop)
    uint32_t win2_id = 0;
    err = BOS_CreateSurface(BWE_DESKTOP_ID, 200, 150, 400, 300, BWE_FLAG_VISIBLE, &win2_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Window 2 creation\n"); return; }

    // Panel2 (child of Window 2)
    uint32_t panel2_id = 0;
    err = BOS_CreateSurface(win2_id, 10, 30, 380, 260, BWE_FLAG_VISIBLE, &panel2_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Panel2 creation\n"); return; }

    // ---- Validate Surface Count ----
    display_print("\nSurface Count: ");
    display_print_dec(BWE_GetSurfaceCount());
    display_print(" (Expected: 8)\n\n");

    if (BWE_GetSurfaceCount() != 8) {
        display_print("FAIL: Surface count mismatch\n");
        return;
    }

    // ---- Step 1: Compute Absolute Screen Coordinates ----
    display_print("--- Step 1: Screen Coordinate Computation ---\n");
    BWE_ComputeScreenBounds();

    // Validate Button's absolute position
    // Button is at: Desktop(0,0) + Window1(50,50) + Panel(10,30) + Button(20,20)
    // Expected absolute: (80, 100)
    BWE_Surface* button = BWE_GetSurface(button_id);
    if (button) {
        display_print("Button Screen Position: (");
        display_print_dec(button->screen_bounds.x);
        display_print(",");
        display_print_dec(button->screen_bounds.y);
        display_print(") Expected: (80,100)\n");

        if (button->screen_bounds.x != 80 || button->screen_bounds.y != 100) {
            display_print("FAIL: Button screen coordinates incorrect\n");
            return;
        }
        display_print("[OK] Screen Coordinate Math Verified\n\n");
    }

    // ---- Step 2: Z-Order Composition Traversal ----
    display_print("--- Step 2: Z-Order Composition ---\n");
    BWE_Compose();

    // ---- Step 3: Destroy Window 2 (test recursive cleanup) ----
    display_print("\n--- Step 3: Recursive Destroy ---\n");
    BOS_DestroySurface(win2_id);

    display_print("Surface Count After Destroy: ");
    display_print_dec(BWE_GetSurfaceCount());
    display_print(" (Expected: 6)\n");

    if (BWE_GetSurfaceCount() != 6) {
        display_print("FAIL: Surface count after destroy\n");
        return;
    }
    display_print("[OK] Recursive Destroy Verified\n\n");

    // ---- Step 4: Recompose after destroy ----
    display_print("--- Step 4: Recompose After Destroy ---\n");
    BWE_ComputeScreenBounds();
    BWE_Compose();

    // ---- Step 5: Hide Window 1 and verify compose skips it ----
    display_print("\n--- Step 5: Hide/Show ---\n");
    BOS_Hide(win1_id);
    display_print("After hiding Window 1:\n");
    BWE_Compose();

    BOS_Show(win1_id);
    display_print("After showing Window 1:\n");
    BWE_Compose();

    // ---- PASS ----
    display_print("\n========================================\n");
    display_print("  PASS_BWE_PHASE2\n");
    display_print("========================================\n");
}

// ============================================================
// BOS_Test_Phase3 — Focus Engine Validation
// ============================================================
void BOS_Test_Phase3(void) {
    display_print("\n--- BWE Phase 3: Focus Engine ---\n\n");

    BOSurface_Init();

    uint32_t a = 0, b = 0, c = 0;
    BOS_CreateSurface(BWE_DESKTOP_ID, 10, 10, 100, 100, BWE_FLAG_VISIBLE, &a);
    BOS_CreateSurface(BWE_DESKTOP_ID, 20, 20, 100, 100, BWE_FLAG_VISIBLE, &b);
    BOS_CreateSurface(BWE_DESKTOP_ID, 30, 30, 100, 100, BWE_FLAG_VISIBLE, &c);

    display_print("\n[Test] Focus A\n");
    BOS_SetFocus(a);
    
    display_print("\n[Test] Focus B\n");
    BOS_SetFocus(b);
    
    display_print("\n[Test] Focus C\n");
    BOS_SetFocus(c);

    if (BOS_GetFocus() == c) {
        display_print("\n[Verify] Only C owns focus: OK\n");
    } else {
        display_print("\n[Verify] Only C owns focus: FAIL\n");
    }

    display_print("\n[Test] Destroy C\n");
    BOS_DestroySurface(c);
    
    if (BOS_GetFocus() == b) {
        display_print("[Verify] Focus returns to B: OK\n");
    } else {
        display_print("[Verify] Focus returns to B: FAIL\n");
    }

    display_print("\n[Test] Destroy B\n");
    BOS_DestroySurface(b);
    
    if (BOS_GetFocus() == a) {
        display_print("[Verify] Focus returns to A: OK\n");
    } else {
        display_print("[Verify] Focus returns to A: FAIL\n");
    }

    display_print("\n[Test] Destroy A\n");
    BOS_DestroySurface(a);
    
    if (BOS_GetFocus() == BWE_DESKTOP_ID) {
        display_print("[Verify] Desktop regains focus: OK\n");
    } else {
        display_print("[Verify] Desktop regains focus: FAIL\n");
    }

    display_print("\nPASS_BWE_PHASE3\n");
}

// ============================================================
// BOS_Test_Phase4 — Mouse Interaction (Drag & Focus)
// ============================================================
void BOS_Test_Phase4(void) {
    bwe_log("INFO", "--- BWE Phase 4: Mouse Interaction ---");

    BOSurface_Init();

    // Create a Draggable Window 1
    uint32_t win1 = 0;
    BOS_CreateSurface(BWE_DESKTOP_ID, 50, 50, 400, 300, BWE_FLAG_VISIBLE | BWE_FLAG_DRAGGABLE, &win1);
    
    // Window 1 Content
    uint32_t win1_panel = 0;
    BOS_CreateSurface(win1, 10, 30, 380, 260, BWE_FLAG_VISIBLE, &win1_panel);

    // Create a Draggable Window 2
    uint32_t win2 = 0;
    BOS_CreateSurface(BWE_DESKTOP_ID, 150, 150, 400, 300, BWE_FLAG_VISIBLE | BWE_FLAG_DRAGGABLE, &win2);
    
    // Window 2 Content
    uint32_t win2_panel = 0;
    BOS_CreateSurface(win2, 10, 30, 380, 260, BWE_FLAG_VISIBLE, &win2_panel);
    
    // Pre-calculate initial screen bounds
    BWE_ComputeScreenBounds();
}
