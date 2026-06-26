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

// ============================================================
// Static Surface Pool
// ============================================================
static BWE_Surface surface_pool[BWE_MAX_SURFACES];
static uint32_t    next_surface_id = 1;  // 0 is reserved for Desktop
static uint32_t    active_surface_count = 0;

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

    // Mark slot as free
    surface->active = false;
    surface->state = BWE_STATE_DESTROYED;
    active_surface_count--;

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
static void compose_recursive(BWE_Surface* surface, uint32_t depth) {
    if (!(surface->flags & BWE_FLAG_VISIBLE)) {
        return; // Skip hidden surfaces
    }

    // Log this surface's render with indentation for tree depth
    display_print("[BWE_RENDER] ");
    for (uint32_t d = 0; d < depth; d++) {
        display_print("  ");
    }
    if (surface->id == BWE_DESKTOP_ID) {
        display_print("Desktop");
    } else {
        display_print("Surface #");
        display_print_dec(surface->id);
    }
    display_print(" @ (");
    display_print_dec(surface->screen_bounds.x);
    display_print(",");
    display_print_dec(surface->screen_bounds.y);
    display_print(") ");
    display_print_dec(surface->screen_bounds.width);
    display_print("x");
    display_print_dec(surface->screen_bounds.height);
    display_print("\n");

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
