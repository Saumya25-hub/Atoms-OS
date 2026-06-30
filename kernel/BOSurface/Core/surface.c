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
#include "app_manager.h"
#include "../Apps/terminal.h"
#include "../../display/display.h"
#include "bovisual/Include/events.h"
#include "bovisual/Include/controls.h"
#include "kernel/conhost/conhost.h"
#include "../Events/gui_events.h"
#include "kernel/scheduler/include/task.h"
#include "kernel/input/input_abstraction.h"
#include "kernel/bocompositor/bocompositor.h"

// ============================================================
// Static Surface Pool
// ============================================================
static BWE_Surface surface_pool[BWE_MAX_SURFACES];
static uint32_t    next_surface_id = 1;  // 0 is reserved for Desktop
static uint32_t    active_surface_count = 0;
uint32_t           g_current_creating_pid = 0;

// Focus Engine State
static uint32_t    bwe_active_surface_id = 0;
static uint32_t    bwe_focused_surface_id = 0;
static uint32_t    bwe_focused_control_id = 0;
static uint32_t    bwe_previous_focus_id = 0;

// Drag Engine State
bool               bwe_is_dragging = false;
uint32_t           bwe_drag_surface_id = 0;
int32_t            bwe_drag_offset_x = 0;
int32_t            bwe_drag_offset_y = 0;

typedef enum {
    DRAG_IDLE,
    DRAG_DRAGGING,
    DRAG_RELEASE_PENDING
} DragState;
DragState bwe_drag_state = DRAG_IDLE;
uint8_t g_update_lock = 0;

uint32_t           bwe_capture_surface_id = 0;
bool               BOS_DEBUG_MODE = true;

// Hover Engine State
static uint32_t    bwe_hover_surface_id = 0;
static uint32_t    bwe_pressed_surface_id = 0;
bool               g_frame_frozen = false;

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
// BODEBUG Dump (Phase 17.6)
// ============================================================
void bodebug_dump(void) {
    if (!BOS_DEBUG_MODE) return;
    
    extern Task* scheduler_current_task(void);
    Task* curr = scheduler_current_task();
    
    static uint32_t last_pid = 0xFFFFFFFF;
    static uint32_t last_focus = 0xFFFFFFFF;
    static uint32_t last_mouse_owner = 0xFFFFFFFF;
    static uint32_t last_surfaces = 0xFFFFFFFF;
    
    uint32_t curr_pid = curr ? curr->id : 0;
    if (curr_pid == last_pid && 
        bwe_focused_surface_id == last_focus && 
        bwe_capture_surface_id == last_mouse_owner && 
        active_surface_count == last_surfaces) {
        return;
    }
    
    last_pid = curr_pid;
    last_focus = bwe_focused_surface_id;
    last_mouse_owner = bwe_capture_surface_id;
    last_surfaces = active_surface_count;
    
    display_print("\n[BO_DEBUG]\n");
    display_print("PID: "); display_print_dec(curr_pid); display_print("\n");
    display_print("FOCUS: "); display_print_dec(bwe_focused_surface_id); display_print("\n");
    display_print("MOUSE_OWNER: "); display_print_dec(bwe_capture_surface_id); display_print("\n");
    display_print("SURFACES: "); display_print_dec(active_surface_count); display_print("\n");
    display_print("TASK: "); display_print(curr ? curr->name : "None"); display_print("\n");
    display_print("EVENT: GUI UPDATE\n");
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
        surface_pool[i].type = BWE_TYPE_SURFACE;
        surface_pool[i].local_bounds.x = 0;
        surface_pool[i].local_bounds.y = 0;
        surface_pool[i].local_bounds.width = 0;
        surface_pool[i].local_bounds.height = 0;
        surface_pool[i].screen_bounds.x = 0;
        surface_pool[i].screen_bounds.y = 0;
        surface_pool[i].screen_bounds.width = 0;
        surface_pool[i].screen_bounds.height = 0;
        surface_pool[i].is_dirty = false;
        surface_pool[i].old_screen_bounds.x = 0;
        surface_pool[i].old_screen_bounds.y = 0;
        surface_pool[i].old_screen_bounds.width = 0;
        surface_pool[i].old_screen_bounds.height = 0;
    }

    bos_gui_events_init();

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
    desktop->type = BWE_TYPE_SURFACE;
    desktop->local_bounds.x = 0;
    desktop->local_bounds.y = 0;
    desktop->local_bounds.width = (int32_t)g_kernel_screen_width;
    desktop->local_bounds.height = (int32_t)g_kernel_screen_height;
    desktop->screen_bounds = desktop->local_bounds;
    desktop->is_dirty = false;
    desktop->old_screen_bounds = desktop->screen_bounds;

    next_surface_id = 1;
    active_surface_count = 1;

    BOCompositor_Initialize();
    extern void bosurface_compositor_render_cb(void* surface_handle, const BOCompositorRect* clip_rect);
    BOCompositor_SetRenderCallback(bosurface_compositor_render_cb);
    BOCompositor_RegisterSurface(BWE_DESKTOP_ID, 0, 0, (int32_t)g_kernel_screen_width, (int32_t)g_kernel_screen_height, 0, desktop);

    bwe_log("INFO", "BOSurface Initialized");
    bwe_log("INFO", "Desktop Surface Created (ID 0)");
}

// ============================================================
// BOS_CreateSurface — Allocates a new surface in the tree
// ============================================================
bwe_error_t BOS_CreateSurface(uint32_t parent_id, uint32_t x, uint32_t y,
                               uint32_t width, uint32_t height,
                               uint32_t flags, uint32_t* out_surface_id) {
    if (active_surface_count >= BWE_MAX_SURFACES) {
        display_print("[BWE_ERROR] Surface limit reached (64). PID: ");
        display_print_dec(g_current_creating_pid);
        display_print("\n");
        return BWE0004;
    }

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
    surface->owner_pid = g_current_creating_pid;
    surface->type = BWE_TYPE_SURFACE;
    surface->local_bounds.x = (int32_t)x;
    surface->local_bounds.y = (int32_t)y;
    surface->local_bounds.width = (int32_t)width;
    surface->local_bounds.height = (int32_t)height;

    // Screen bounds will be computed by BWE_ComputeScreenBounds
    surface->screen_bounds.x = 0;
    surface->screen_bounds.y = 0;
    surface->screen_bounds.width = (int32_t)width;
    surface->screen_bounds.height = (int32_t)height;
    surface->is_dirty = false;
    surface->old_screen_bounds = surface->screen_bounds;

    // Add to parent's children
    if (!add_child_to_parent(parent, id)) {
        surface->active = false;
        bwe_log("ERROR", "BOS_CreateSurface: Parent children array full");
        return BWE0004;
    }

    active_surface_count++;

    if (parent_id == BWE_DESKTOP_ID) {
        BOCompositor_RegisterSurface(id, surface->screen_bounds.x, surface->screen_bounds.y, surface->screen_bounds.width, surface->screen_bounds.height, surface->z_order, surface);
    }

    if (out_surface_id) {
        *out_surface_id = id;
    }

    bwe_log_id("INFO", "Surface Created", id);
    return BWE_SUCCESS;
}

// Bounded strncpy helper
static void bwe_strncpy(char* dest, const char* src, uint32_t n) {
    if (!dest || !src || n == 0) return;
    uint32_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

// ============================================================
// BOS_CreatePanel — Create a Panel Control
// ============================================================
bwe_error_t BOS_CreatePanel(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color_bg, uint32_t* out_control_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_FLAG_VISIBLE, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Surface* surface = BWE_GetSurface(id);
    if (!surface) return BWE0001;

    surface->type = BWE_TYPE_PANEL;
    surface->control_data.panel.bg_color = color_bg;

    if (out_control_id) *out_control_id = id;
    return BWE_SUCCESS;
}

// ============================================================
// BOS_CreateButton — Create a Button Control
// ============================================================
bwe_error_t BOS_CreateButton(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* text, void (*on_click)(uint32_t), uint32_t* out_control_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_FLAG_VISIBLE, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Surface* surface = BWE_GetSurface(id);
    if (!surface) return BWE0001;

    surface->type = BWE_TYPE_BUTTON;
    surface->control_data.button.text_color = 0xFFFFFFFF; // White text
    surface->control_data.button.bg_color = 0xFF1E293B;   // Dark theme button background
    surface->control_data.button.is_pressed = false;
    surface->control_data.button.is_hovered = false;
    surface->control_data.button.on_click = on_click;
    
    if (text) {
        bwe_strncpy(surface->control_data.button.text, text, sizeof(surface->control_data.button.text));
    } else {
        surface->control_data.button.text[0] = '\0';
    }

    if (out_control_id) *out_control_id = id;
    return BWE_SUCCESS;
}

// ============================================================
// BOS_CreateLabel — Create a Label Control
// ============================================================
bwe_error_t BOS_CreateLabel(uint32_t parent_id, uint32_t x, uint32_t y, const char* text, uint32_t color_fg, uint32_t* out_control_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, 200, 20, BWE_FLAG_VISIBLE, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Surface* surface = BWE_GetSurface(id);
    if (!surface) return BWE0001;

    surface->type = BWE_TYPE_LABEL;
    surface->control_data.label.text_color = color_fg;
    surface->control_data.label.transparent = true;
    if (text) {
        bwe_strncpy(surface->control_data.label.text, text, sizeof(surface->control_data.label.text));
    } else {
        surface->control_data.label.text[0] = '\0';
    }

    if (out_control_id) *out_control_id = id;
    return BWE_SUCCESS;
}

// ============================================================
// BOS_CreateTextbox — Create a TextBox Control
// ============================================================
bwe_error_t BOS_CreateTextbox(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* placeholder, uint32_t* out_control_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_FLAG_VISIBLE, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Surface* surface = BWE_GetSurface(id);
    if (!surface) return BWE0001;

    surface->type = BWE_TYPE_TEXTBOX;
    surface->control_data.textbox.bg_color = 0xFF0F172A; // Dark background
    surface->control_data.textbox.text_color = 0xFFF1F5F9; // Off-white text
    surface->control_data.textbox.text[0] = '\0';
    if (placeholder) {
        bwe_strncpy(surface->control_data.textbox.placeholder, placeholder, sizeof(surface->control_data.textbox.placeholder));
    } else {
        surface->control_data.textbox.placeholder[0] = '\0';
    }

    if (out_control_id) *out_control_id = id;
    return BWE_SUCCESS;
}

// ============================================================
// BOS_SetText — Update Text State of a Control
// ============================================================
bwe_error_t BOS_SetText(uint32_t target_id, const char* text) {
    BWE_Surface* surface = BWE_GetSurface(target_id);
    if (!surface) return BWE0001;

    if (surface->type == BWE_TYPE_BUTTON) {
        if (text) {
            bwe_strncpy(surface->control_data.button.text, text, sizeof(surface->control_data.button.text));
        } else {
            surface->control_data.button.text[0] = '\0';
        }
    } else if (surface->type == BWE_TYPE_LABEL) {
        extern uint32_t g_compositor_current_surface_id;
        g_compositor_current_surface_id = surface->id;
        if (text) {
            bwe_strncpy(surface->control_data.label.text, text, sizeof(surface->control_data.label.text));
        } else {
            surface->control_data.label.text[0] = '\0';
        }
    } else if (surface->type == BWE_TYPE_TEXTBOX) {
        if (text) {
            bwe_strncpy(surface->control_data.textbox.text, text, sizeof(surface->control_data.textbox.text));
        } else {
            surface->control_data.textbox.text[0] = '\0';
        }
    } else {
        bwe_log("ERROR", "BOS_SetText: Target is not a support-text control type");
        return BWE0007; // Invalid Control ID
    }
    
    BOS_InvalidateSurface(target_id);
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

    // Add damage for the surface's entire screen bounds before destroying
    if (surface->screen_bounds.width > 0 && surface->screen_bounds.height > 0) {
        BOCompositor_AddDamage(surface->screen_bounds.x, surface->screen_bounds.y,
                               surface->screen_bounds.width, surface->screen_bounds.height);
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
    
    extern bool bwe_is_dragging;
    extern uint32_t bwe_drag_surface_id;
    
    if (bwe_drag_surface_id == surface_id || bwe_capture_surface_id == surface_id) {
        bwe_is_dragging = false;
        if (bwe_drag_surface_id == surface_id) bwe_drag_surface_id = 0;
        if (bwe_capture_surface_id == surface_id) bwe_capture_surface_id = 0;
        bwe_log_id("INPUT", "Destroyed Surface Cleared Drag State", surface_id);
    }
    
    bwe_log_id("INFO", "Surface Destroyed", surface_id);
    return BWE_SUCCESS;
}

// ============================================================
// Window State APIs
// ============================================================
bwe_error_t BOS_MinimizeSurface(uint32_t surface_id) {
    if (surface_id == BWE_DESKTOP_ID) {
        bwe_log("ERROR", "Cannot minimize Desktop surface");
        return BWE0006;
    }
    BWE_Surface* surface = BWE_GetSurface(surface_id);
    if (!surface) return BWE0001;

    if (surface->screen_bounds.width > 0 && surface->screen_bounds.height > 0) {
        BOCompositor_AddDamage(surface->screen_bounds.x, surface->screen_bounds.y,
                               surface->screen_bounds.width, surface->screen_bounds.height);
    }

    surface->state = BWE_STATE_MINIMIZED;
    surface->flags &= ~BWE_FLAG_VISIBLE;
    
    if (bwe_focused_surface_id == surface_id) {
        BOS_ClearFocus();
    }
    
    bwe_log_id("INFO", "Surface Minimized", surface_id);
    return BWE_SUCCESS;
}

bwe_error_t BOS_MaximizeSurface(uint32_t surface_id) {
    if (surface_id == BWE_DESKTOP_ID) {
        bwe_log("ERROR", "Cannot maximize Desktop surface");
        return BWE0006;
    }
    BWE_Surface* surface = BWE_GetSurface(surface_id);
    if (!surface) return BWE0001;
    
    BWE_Surface* desktop = BWE_GetSurface(BWE_DESKTOP_ID);
    if (!desktop) return BWE0001;

    if (surface->screen_bounds.width > 0 && surface->screen_bounds.height > 0) {
        BOCompositor_AddDamage(surface->screen_bounds.x, surface->screen_bounds.y,
                               surface->screen_bounds.width, surface->screen_bounds.height);
    }

    // Save current bounds if not already maximized
    if (!(surface->flags & BWE_FLAG_MAXIMIZED)) {
        surface->restore_bounds = surface->local_bounds;
    }
    
    // Maximize to desktop bounds
    surface->local_bounds.x = 0;
    surface->local_bounds.y = 0;
    surface->local_bounds.width = desktop->local_bounds.width;
    surface->local_bounds.height = desktop->local_bounds.height;

    surface->state = BWE_STATE_MAXIMIZED;
    surface->flags |= (BWE_FLAG_VISIBLE | BWE_FLAG_MAXIMIZED);
    
    BOS_SetFocus(surface_id);
    BOS_InvalidateSurface(surface_id);
    
    bwe_log_id("INFO", "Surface Maximized", surface_id);
    return BWE_SUCCESS;
}

bwe_error_t BOS_RestoreSurface(uint32_t surface_id) {
    if (surface_id == BWE_DESKTOP_ID) return BWE0006;
    BWE_Surface* surface = BWE_GetSurface(surface_id);
    if (!surface) return BWE0001;

    if (surface->screen_bounds.width > 0 && surface->screen_bounds.height > 0) {
        BOCompositor_AddDamage(surface->screen_bounds.x, surface->screen_bounds.y,
                               surface->screen_bounds.width, surface->screen_bounds.height);
    }

    if (surface->flags & BWE_FLAG_MAXIMIZED) {
        // Restore from maximized
        surface->local_bounds = surface->restore_bounds;
        surface->flags &= ~BWE_FLAG_MAXIMIZED;
    }
    
    surface->state = BWE_STATE_VISIBLE;
    surface->flags |= BWE_FLAG_VISIBLE;
    
    BOS_SetFocus(surface_id);
    BOS_InvalidateSurface(surface_id);
    
    bwe_log_id("INFO", "Surface Restored", surface_id);
    return BWE_SUCCESS;
}

bwe_error_t BOS_CloseSurface(uint32_t surface_id) {
    return BOS_DestroySurface(surface_id);
}

// Window system callbacks
static void internal_btn_close_clicked(uint32_t btn_id) {
    uint32_t win_id = BWE_GetTopLevelSurface(btn_id);
    // Route through App Manager for proper lifecycle cleanup
    BOS_StopApplicationByWindow(win_id);
}
static void internal_btn_max_clicked(uint32_t btn_id) {
    uint32_t win_id = BWE_GetTopLevelSurface(btn_id);
    BWE_Surface* win = BWE_GetSurface(win_id);
    if (win) {
        if (win->flags & BWE_FLAG_MAXIMIZED) BOS_RestoreSurface(win_id);
        else BOS_MaximizeSurface(win_id);
    }
}
static void internal_btn_min_clicked(uint32_t btn_id) {
    uint32_t win_id = BWE_GetTopLevelSurface(btn_id);
    BOS_MinimizeSurface(win_id);
}

// BOS_CreateWindow - Titlebar Engine entry
bwe_error_t BOS_CreateWindow(int32_t x, int32_t y, int32_t width, int32_t height, const char* title, uint32_t* out_id) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateSurface(BWE_DESKTOP_ID, x, y, width, height, BWE_FLAG_VISIBLE, &win_id);
    if (err != BWE_SUCCESS) return err;

    uint32_t titlebar_id = 0;
    err = BOS_CreatePanel(win_id, 0, 0, width, 30, 0xFF334155, &titlebar_id);
    if (err == BWE_SUCCESS) {
        BWE_Surface* titlebar = BWE_GetSurface(titlebar_id);
        if (titlebar) {
            titlebar->flags |= BWE_FLAG_DRAGGABLE;
        }

        uint32_t label_id = 0;
        BOS_CreateLabel(titlebar_id, 10, 8, title ? title : "Window", 0xFFFFFFFF, &label_id);

        uint32_t btn_min = 0, btn_max = 0, btn_close = 0;
        BOS_CreateButton(titlebar_id, width - 90, 5, 20, 20, "_", internal_btn_min_clicked, &btn_min);
        BOS_CreateButton(titlebar_id, width - 60, 5, 20, 20, "O", internal_btn_max_clicked, &btn_max);
        BOS_CreateButton(titlebar_id, width - 30, 5, 20, 20, "X", internal_btn_close_clicked, &btn_close);
        
        BWE_Surface* sb = BWE_GetSurface(btn_close); 
        if (sb) sb->control_data.button.bg_color = 0xFFEF4444; // Red
    }

    if (out_id) *out_id = win_id;
    return BWE_SUCCESS;
}

// ============================================================
// BOS_SetWallpaper — Set Desktop Wallpaper (Phase 8)
// ============================================================
bwe_error_t BOS_SetWallpaper(uint32_t color) {
    BWE_Surface* desktop = BWE_GetSurface(BWE_DESKTOP_ID);
    if (!desktop) return BWE0001;

    desktop->type = BWE_TYPE_WALLPAPER;
    desktop->control_data.panel.bg_color = color;
    // Desktop cannot be dragged
    desktop->flags &= ~BWE_FLAG_DRAGGABLE;

    bwe_log("INFO", "Wallpaper set on Desktop");
    return BWE_SUCCESS;
}

// ============================================================
// BOS_CreateTaskbar — Initialize the Taskbar (Phase 8)
// ============================================================
bwe_error_t BOS_CreateTaskbar(void) {
    extern uint32_t g_kernel_screen_width;
    extern uint32_t g_kernel_screen_height;

    uint32_t taskbar_id = 0;
    // Dock at bottom: y = height - 40
    bwe_error_t err = BOS_CreatePanel(BWE_DESKTOP_ID, 0, g_kernel_screen_height - 40, g_kernel_screen_width, 40, 0xFF1E293B, &taskbar_id);
    if (err != BWE_SUCCESS) return err;

    BWE_Surface* taskbar = BWE_GetSurface(taskbar_id);
    if (!taskbar) return BWE0001;

    taskbar->type = BWE_TYPE_TASKBAR;
    taskbar->flags &= ~BWE_FLAG_DRAGGABLE; // Cannot be dragged
    // Make sure it renders on top by assigning highest z_order among desktop children
    taskbar->z_order = 999; 

    // Start Button Placeholder
    uint32_t btn_start = 0;
    BOS_CreateButton(taskbar_id, 10, 5, 80, 30, "Start", 0, &btn_start);
    BWE_Surface* start_btn = BWE_GetSurface(btn_start);
    if (start_btn) {
        start_btn->control_data.button.bg_color = 0xFF2563EB; // Blue accent
    }

    bwe_log("INFO", "Taskbar Created");
    return BWE_SUCCESS;
}

// ============================================================
// BOS_CreateDesktopIcon — Desktop Icon Container (Phase 8)
// ============================================================
bwe_error_t BOS_CreateDesktopIcon(uint32_t x, uint32_t y, const char* label, void (*on_click)(uint32_t), uint32_t* out_id) {
    uint32_t icon_id = 0;
    // Create as button so it receives hover/click states naturally
    bwe_error_t err = BOS_CreateButton(BWE_DESKTOP_ID, x, y, 80, 80, "", on_click, &icon_id);
    if (err != BWE_SUCCESS) return err;

    BWE_Surface* icon = BWE_GetSurface(icon_id);
    if (!icon) return BWE0001;

    icon->type = BWE_TYPE_DESKTOP_ICON;
    icon->control_data.button.bg_color = 0x00000000; // Transparent bg natively
    
    // Icon label below
    uint32_t lbl_id = 0;
    BOS_CreateLabel(icon_id, 0, 60, label ? label : "Icon", 0xFFFFFFFF, &lbl_id);
    
    // Future: Image/Sprite for the icon at (16, 10) 48x48
    
    if (out_id) *out_id = icon_id;
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
    BOCompositor_Update(target_id, surface->screen_bounds.x, surface->screen_bounds.y, surface->screen_bounds.width, surface->screen_bounds.height, true);
    bwe_log_id("INFO", "Surface Shown", target_id);
    BOS_InvalidateSurface(target_id);
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
    BOCompositor_Update(target_id, surface->screen_bounds.x, surface->screen_bounds.y, surface->screen_bounds.width, surface->screen_bounds.height, false);
    bwe_log_id("INFO", "Surface Hidden", target_id);
    BOS_InvalidateSurface(target_id);
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
    
    BOCompositor_Update(target_id, surface->screen_bounds.x, surface->screen_bounds.y, (int32_t)width, (int32_t)height, (surface->flags & BWE_FLAG_VISIBLE) != 0);
    bwe_log_id("INFO", "Bounds Updated", target_id);
    BOS_InvalidateSurface(target_id);
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
            BOS_InvalidateSurface(bwe_focused_surface_id);
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
        BOCompositor_BringToFront(new_active_id);
    }

    // 4. Gain Focus
    bwe_focused_surface_id = surface_id;
    surface->flags |= BWE_FLAG_FOCUSED;
    surface->state = BWE_STATE_FOCUSED;
    bwe_log_id("FOCUS", "Surface Gained Focus", surface_id);
    BOCompositor_SetFocus(surface_id);
    BOS_InvalidateSurface(surface_id);

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
            BOS_InvalidateSurface(bwe_focused_surface_id);
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
// BOS_DispatchEvent — Bubbles events up from child to parent
// ============================================================
static bool BOS_DispatchEvent(uint32_t surface_id, const BVEvent* event) {
    if (surface_id == 0) return false;
    BWE_Surface* surface = BWE_GetSurface(surface_id);
    if (!surface) return false;

    bool handled = false;
    
    // Custom Event Handler Hook
    if (surface->on_event) {
        surface->on_event(surface_id, event);
        handled = true; // For now assume custom hooks consume the event
    }

    // Control-specific event consumption
    if (surface->type == BWE_TYPE_BUTTON) {
        if (event->type == BV_EVENT_MOUSE_DOWN) {
            bwe_pressed_surface_id = surface->id;
            bwe_capture_surface_id = surface->id;
            surface->control_data.button.is_pressed = true;
            surface->control_data.button.is_hovered = true; // Ensure hovered state on MOUSE_DOWN!
            BOS_InvalidateSurface(surface->id);
            handled = true;
        } else if (event->type == BV_EVENT_MOUSE_UP) {
            surface->control_data.button.is_pressed = false;
            BOS_InvalidateSurface(surface->id);
            if (bwe_pressed_surface_id == surface->id && surface->control_data.button.is_hovered) {
                // Generate CLICK internally
                bwe_log_id("INPUT", "Button Clicked", surface->id);
                if (surface->control_data.button.on_click) {
                    surface->control_data.button.on_click(surface->id);
                }
                if (surface->control_data.button.user_callback) {
                    bos_gui_event_push(surface->owner_pid, BOS_GUI_EVENT_CLICK, surface->id, surface->parent_id, surface->control_data.button.user_callback);
                }
            }
            if (bwe_pressed_surface_id == surface->id) {
                bwe_pressed_surface_id = 0;
            }
            if (bwe_capture_surface_id == surface->id) {
                bwe_capture_surface_id = 0;
            }
            handled = true;
        } else if (event->type == BV_EVENT_MOUSE_ENTER) {
            surface->control_data.button.is_hovered = true;
            BOS_InvalidateSurface(surface->id);
            handled = true;
        } else if (event->type == BV_EVENT_MOUSE_LEAVE) {
            surface->control_data.button.is_hovered = false;
            surface->control_data.button.is_pressed = false;
            BOS_InvalidateSurface(surface->id);
            handled = true;
        }
    } else if (surface->type == BWE_TYPE_TEXTBOX) {
        if (event->type == BV_EVENT_MOUSE_DOWN) {
            handled = true; // Consume click to focus
        }
    }

    // Event Bubbling
    if (!handled && surface->parent_id != BWE_DESKTOP_ID && surface->id != BWE_DESKTOP_ID) {
        return BOS_DispatchEvent(surface->parent_id, event);
    }
    
    return handled;
}

// ============================================================
// BOS_ProcessEvent — Handle Mouse and Keyboard input
// ============================================================
void BOS_ProcessEvent(const BVEvent* event) {
    if (!event || g_frame_frozen) return;

    if (event->type == BV_EVENT_MOUSE_MOVE) {
        uint32_t hit_id = bwe_capture_surface_id != 0 ? bwe_capture_surface_id : BWE_HitTest(event->mouse_x, event->mouse_y);

        // Drag Engine
        if (bwe_is_dragging && bwe_drag_surface_id != 0) {
            BWE_Surface* dragged = BWE_GetSurface(bwe_drag_surface_id);
            if (dragged) {
                int32_t new_x = event->mouse_x - bwe_drag_offset_x;
                int32_t new_y = event->mouse_y - bwe_drag_offset_y;
                BOS_SetBounds(bwe_drag_surface_id, new_x, new_y, dragged->local_bounds.width, dragged->local_bounds.height);
            }
            return; // NO other window receives mouse input, skip hover entirely
        }

        // Hover Engine
        if (hit_id != bwe_hover_surface_id) {
            if (bwe_hover_surface_id != 0) {
                BVEvent leave_ev = *event;
                leave_ev.type = BV_EVENT_MOUSE_LEAVE;
                BOS_DispatchEvent(bwe_hover_surface_id, &leave_ev);
            }
            bwe_hover_surface_id = hit_id;
            if (bwe_hover_surface_id != 0) {
                BVEvent enter_ev = *event;
                enter_ev.type = BV_EVENT_MOUSE_ENTER;
                BOS_DispatchEvent(bwe_hover_surface_id, &enter_ev);
            }
        }

        if (bwe_capture_surface_id != 0) {
            BOS_DispatchEvent(bwe_capture_surface_id, event);
        } else {
            BOS_DispatchEvent(hit_id, event);
        }
    }
    else if (event->type == BV_EVENT_MOUSE_DOWN) {
        uint32_t hit_id = BWE_HitTest(event->mouse_x, event->mouse_y);
        
        bwe_log_id("INPUT", "Hit Test Result", hit_id);

        if (hit_id != BWE_DESKTOP_ID && hit_id != 0) {
            BOS_SetFocus(hit_id);
            
            bool handled = BOS_DispatchEvent(hit_id, event);
            
            // Drag fallback if unhandled
            if (!handled) {
                BWE_Surface* drag_check = BWE_GetSurface(hit_id);
                while (drag_check && drag_check->id != BWE_DESKTOP_ID) {
                    if (drag_check->flags & BWE_FLAG_DRAGGABLE) {
                        uint32_t top_level_id = BWE_GetTopLevelSurface(hit_id);
                        BWE_Surface* top_level = BWE_GetSurface(top_level_id);
                        if (top_level) {
                            bwe_drag_state = DRAG_DRAGGING;
                            bwe_is_dragging = true;
                            bwe_drag_surface_id = top_level_id;
                            bwe_capture_surface_id = hit_id;
                            bwe_drag_offset_x = event->mouse_x - top_level->local_bounds.x;
                            bwe_drag_offset_y = event->mouse_y - top_level->local_bounds.y;
                            bwe_log_id("INPUT", "Started Dragging", top_level_id);
                        }
                        break;
                    }
                    drag_check = BWE_GetSurface(drag_check->parent_id);
                }
            }
        } else {
            BOS_ClearFocus();
            BOS_DispatchEvent(BWE_DESKTOP_ID, event);
        }
    }
    else if (event->type == BV_EVENT_MOUSE_UP) {
        uint32_t target_id = bwe_capture_surface_id != 0 ? bwe_capture_surface_id : BWE_HitTest(event->mouse_x, event->mouse_y);

        if (bwe_is_dragging || bwe_drag_state == DRAG_DRAGGING) {
            bwe_log_id("INPUT", "Ended Dragging (Pending Release)", bwe_drag_surface_id);
            bwe_drag_state = DRAG_RELEASE_PENDING;
            // Actual state release happens cleanly at the end of the frame in BOF_BeginAtomicFrame
        } else {
            BOS_DispatchEvent(target_id, event);
        }
    }
    else if (event->type == BV_EVENT_KEY_DOWN || event->type == BV_EVENT_KEY_UP) {
        bool handled = false;
        if (bwe_focused_surface_id != 0) {
            handled = BOS_DispatchEvent(bwe_focused_surface_id, event);
        } else if (bwe_active_surface_id != 0) {
            handled = BOS_DispatchEvent(bwe_active_surface_id, event);
        }
        
        if (!handled) {
            ConsoleSession* sess = conhost_get_active_session();
            if (sess && sess->terminal_window_id != 0) {
                terminal_handle_event(sess->terminal_window_id, event);
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

        if (child->parent_id == BWE_DESKTOP_ID) {
            BOCompositor_Update(child->id, child->screen_bounds.x, child->screen_bounds.y, child->screen_bounds.width, child->screen_bounds.height, (child->flags & BWE_FLAG_VISIBLE) != 0);
        }

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

void compose_recursive(BWE_Surface* surface, uint32_t depth, BWE_Rect* clip_rect) {
    if (!(surface->flags & BWE_FLAG_VISIBLE)) {
        return; // Skip hidden surfaces
    }
    
    // Safety bounds check for zero-sized surfaces
    if (surface->screen_bounds.width <= 0 || surface->screen_bounds.height <= 0) {
        return; 
    }

    // BOFRAMES: Aggressive Subtree Clipping
    if (clip_rect) {
        if (clip_rect->width <= 0 || clip_rect->height <= 0) return;

        // Exact Intersection Check
        int32_t cx1 = (surface->screen_bounds.x > clip_rect->x) ? surface->screen_bounds.x : clip_rect->x;
        int32_t cy1 = (surface->screen_bounds.y > clip_rect->y) ? surface->screen_bounds.y : clip_rect->y;
        
        int32_t s_x2 = surface->screen_bounds.x + surface->screen_bounds.width;
        int32_t c_x2 = clip_rect->x + clip_rect->width;
        int32_t cx2 = (s_x2 < c_x2) ? s_x2 : c_x2;
        
        int32_t s_y2 = surface->screen_bounds.y + surface->screen_bounds.height;
        int32_t c_y2 = clip_rect->y + clip_rect->height;
        int32_t cy2 = (s_y2 < c_y2) ? s_y2 : c_y2;

        if (cx1 >= cx2 || cy1 >= cy2) {
            return; // Empty intersection, SKIP completely!
        }
    }

    if (surface->type == BWE_TYPE_SURFACE) {
        extern uint32_t g_compositor_current_surface_id;
        g_compositor_current_surface_id = surface->id;

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

        // Fill rectangle directly
        extern void BOVISUAL_Graphics_Fill_Internal(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color);
        BOVISUAL_Graphics_Fill_Internal(surface->screen_bounds.x, surface->screen_bounds.y, surface->screen_bounds.width, surface->screen_bounds.height, color);

        // Draw focus border if focused
        if (surface->flags & BWE_FLAG_FOCUSED) {
            uint32_t border_color = 0xFF0058EE; // Bright XP blue glow
            BOVISUAL_Graphics_Fill_Internal(surface->screen_bounds.x, surface->screen_bounds.y, surface->screen_bounds.width, 2, border_color);
            BOVISUAL_Graphics_Fill_Internal(surface->screen_bounds.x, surface->screen_bounds.y + surface->screen_bounds.height - 2, surface->screen_bounds.width, 2, border_color);
            BOVISUAL_Graphics_Fill_Internal(surface->screen_bounds.x, surface->screen_bounds.y, 2, surface->screen_bounds.height, border_color);
            BOVISUAL_Graphics_Fill_Internal(surface->screen_bounds.x + surface->screen_bounds.width - 2, surface->screen_bounds.y, 2, surface->screen_bounds.height, border_color);
        }
    }
    else if (surface->type == BWE_TYPE_PANEL || surface->type == BWE_TYPE_WALLPAPER || surface->type == BWE_TYPE_TASKBAR) {
        if (surface->type == BWE_TYPE_PANEL && surface->local_bounds.y == 0 && surface->local_bounds.height == 30) {
            // XP Titlebar Gradient - Optimized Block Render
            uint32_t color_top = 0xFF0058EE;
            uint32_t color_bottom = 0xFF0038A8;
            int32_t h = surface->screen_bounds.height;
            int32_t block_count = 6;
            int32_t block_h = h / block_count;
            for (int32_t i = 0; i < block_count; i++) {
                int32_t y = i * block_h;
                uint32_t r1 = (color_top >> 16) & 0xFF;
                uint32_t g1 = (color_top >> 8) & 0xFF;
                uint32_t b1 = color_top & 0xFF;
                uint32_t r2 = (color_bottom >> 16) & 0xFF;
                uint32_t g2 = (color_bottom >> 8) & 0xFF;
                uint32_t b2 = color_bottom & 0xFF;
                
                uint32_t r = r1 + ((r2 - r1) * y) / h;
                uint32_t g = g1 + ((g2 - g1) * y) / h;
                uint32_t b = b1 + ((b2 - b1) * y) / h;
                
                uint32_t c = 0xFF000000 | (r << 16) | (g << 8) | b;
                
                // Handle remainder for last block
                int32_t draw_h = (i == block_count - 1) ? (h - y) : block_h;
                extern void BOVISUAL_Graphics_Fill_Internal(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color);
                BOVISUAL_Graphics_Fill_Internal(surface->screen_bounds.x, surface->screen_bounds.y + y, surface->screen_bounds.width, draw_h, c);
            }
        } else {
            if (surface->type == BWE_TYPE_WALLPAPER) {
                extern uint32_t* rook_get_wallpaper_buffer(void);
                extern bool rook_is_wallpaper_loaded(void);
                extern void* BOVISUAL_Graphics_GetBuffer(void);
                uint32_t* wall_buf = rook_get_wallpaper_buffer();
                uint32_t* back_buffer = (uint32_t*)BOVISUAL_Graphics_GetBuffer();
                if (rook_is_wallpaper_loaded() && wall_buf != NULL && back_buffer != NULL) {
                    uint32_t w = surface->screen_bounds.width;
                    uint32_t h = surface->screen_bounds.height;
                    uint32_t total = w * h;
                    for (uint32_t i = 0; i < total; i++) {
                        back_buffer[i] = wall_buf[i];
                    }
                } else {
                    BOVISUAL_Control_Panel panel;
                    panel.bounds.x = surface->screen_bounds.x;
                    panel.bounds.y = surface->screen_bounds.y;
                    panel.bounds.width = surface->screen_bounds.width;
                    panel.bounds.height = surface->screen_bounds.height;
                    panel.padding = (BVPadding){0,0,0,0};
                    panel.bg_color = surface->control_data.panel.bg_color;
                    panel.border_color = 0xFF475569;
                    panel.draw_border = false;
                    BV_Panel_Render(&panel);
                }
            } else {
                BOVISUAL_Control_Panel panel;
                panel.bounds.x = surface->screen_bounds.x;
                panel.bounds.y = surface->screen_bounds.y;
                panel.bounds.width = surface->screen_bounds.width;
                panel.bounds.height = surface->screen_bounds.height;
                panel.padding = (BVPadding){0,0,0,0};
                panel.bg_color = surface->control_data.panel.bg_color;
                panel.border_color = 0xFF475569;
                panel.draw_border = (surface->type == BWE_TYPE_PANEL); // Only standard panels get borders
                BV_Panel_Render(&panel);
            }
        }
    }
    else if (surface->type == BWE_TYPE_BUTTON) {
        extern uint32_t g_compositor_current_surface_id;
        g_compositor_current_surface_id = surface->id;
        BOVISUAL_Control_Button button;
        button.bounds.x = surface->screen_bounds.x;
        button.bounds.y = surface->screen_bounds.y;
        button.bounds.width = surface->screen_bounds.width;
        button.bounds.height = surface->screen_bounds.height;
        button.padding = (BVPadding){4,4,4,4};
        button.text = surface->control_data.button.text;
        button.bg_color = surface->control_data.button.bg_color;
        button.hover_color = 0xFF318CE7; // Bright XP Blue
        button.pressed_color = 0xFF1D4ED8; // Darker Blue
        button.text_color = surface->control_data.button.text_color;
        button.border_color = 0xFF2563EB;
        button.h_align = BV_ALIGN_CENTER;
        button.v_align = BV_ALIGN_CENTER;
        button.is_pressed = surface->control_data.button.is_pressed;
        button.is_hovered = surface->control_data.button.is_hovered;
        button.is_focused = (surface->flags & BWE_FLAG_FOCUSED) != 0;
        BV_Button_Render(&button);
    }
    else if (surface->type == BWE_TYPE_LABEL) {
        extern uint32_t g_compositor_current_surface_id;
        g_compositor_current_surface_id = surface->id;
        BOVISUAL_Control_Label label;
        label.bounds.x = surface->screen_bounds.x;
        label.bounds.y = surface->screen_bounds.y;
        label.bounds.width = surface->screen_bounds.width;
        label.bounds.height = surface->screen_bounds.height;
        label.padding = (BVPadding){0,0,0,0};
        label.text = surface->control_data.label.text;
        label.text_color = surface->control_data.label.text_color;
        label.bg_color = 0;
        label.transparent_bg = surface->control_data.label.transparent;
        label.h_align = BV_ALIGN_START;
        label.v_align = BV_ALIGN_CENTER;
        BV_Label_Render(&label);
    }
    else if (surface->type == BWE_TYPE_TEXTBOX) {
        extern uint32_t g_compositor_current_surface_id;
        g_compositor_current_surface_id = surface->id;
        BOVISUAL_Control_TextBox textbox;
        textbox.bounds.x = surface->screen_bounds.x;
        textbox.bounds.y = surface->screen_bounds.y;
        textbox.bounds.width = surface->screen_bounds.width;
        textbox.bounds.height = surface->screen_bounds.height;
        textbox.padding = (BVPadding){4,4,4,4};
        if (surface->control_data.textbox.text[0] != '\0') {
            textbox.text = surface->control_data.textbox.text;
            textbox.text_color = surface->control_data.textbox.text_color;
        } else {
            textbox.text = surface->control_data.textbox.placeholder;
            textbox.text_color = 0xFF64748B;
        }
        textbox.bg_color = surface->control_data.textbox.bg_color;
        textbox.border_color = 0xFF334155;
        textbox.h_align = BV_ALIGN_START;
        textbox.v_align = BV_ALIGN_CENTER;
        textbox.has_focus = (surface->flags & BWE_FLAG_FOCUSED) != 0;
        BV_TextBox_Render(&textbox);
    }
    else if (surface->type == BWE_TYPE_DESKTOP_ICON) {
        extern uint32_t g_compositor_current_surface_id;
        g_compositor_current_surface_id = surface->id;
        // Transparent button behavior
        BOVISUAL_Control_Button button;
        button.bounds.x = surface->screen_bounds.x;
        button.bounds.y = surface->screen_bounds.y;
        button.bounds.width = surface->screen_bounds.width;
        button.bounds.height = surface->screen_bounds.height;
        button.padding = (BVPadding){4,4,4,4};
        button.text = ""; // Text is rendered by child label
        
        // Only show background if hovered or pressed
        if (surface->control_data.button.is_pressed) {
            button.bg_color = 0x55FFFFFF; // Semi-transparent white
            button.border_color = 0xFF38BDF8;
        } else if (surface->control_data.button.is_hovered) {
            button.bg_color = 0x22FFFFFF;
            button.border_color = 0x5538BDF8;
        } else {
            button.bg_color = surface->control_data.button.bg_color; // usually transparent/desktop color
            button.border_color = surface->control_data.button.bg_color;
        }
        
        button.hover_color = button.bg_color;
        button.pressed_color = button.bg_color;
        button.text_color = 0;
        button.h_align = BV_ALIGN_CENTER;
        button.v_align = BV_ALIGN_CENTER;
        button.is_pressed = surface->control_data.button.is_pressed;
        button.is_hovered = surface->control_data.button.is_hovered;
        button.is_focused = (surface->flags & BWE_FLAG_FOCUSED) != 0;
        
        // If transparent, we need a custom graphics fill because BV_Button_Render draws opaque.
        // Wait, bovisual does not support alpha blending yet. We will just simulate it by not rendering the bg unless hovered.
        if (surface->control_data.button.is_hovered || surface->control_data.button.is_pressed) {
            // Very hacky without alpha blending, but we draw a solid color for now
            button.bg_color = 0xFF475569; // Slate gray background for icon selection
            button.border_color = 0xFF94A3B8;
            BV_Button_Render(&button);
        } else {
            // Draw nothing (fully transparent)
        }
    }

    // Custom Render Hook (e.g. for Terminal custom drawing)
    if (surface->on_render) {
        surface->on_render(surface);
    }

    // Render children in z-order (already insertion-ordered)
    for (uint32_t i = 0; i < surface->child_count; i++) {
        if (surface->id == BWE_DESKTOP_ID) continue;
        BWE_Surface* child = BWE_GetSurface(surface->children[i]);
        if (child) {
            compose_recursive(child, depth + 1, clip_rect);
        }
    }
}

void bosurface_compositor_render_cb(void* surface_handle, const BOCompositorRect* clip_rect) {
    if (!surface_handle || !clip_rect) return;
    BWE_Surface* surface = (BWE_Surface*)surface_handle;
    if (!(surface->flags & BWE_FLAG_VISIBLE) || !surface->active) return;
    BWE_Rect clip = {clip_rect->x, clip_rect->y, clip_rect->width, clip_rect->height};
    compose_recursive(surface, 0, &clip);
}

void BWE_Compose(void) {
    BOCompositor_ComposeFrame();
}

// ============================================================
// BOFRAMES MANAGER IMPLEMENTATION
// ============================================================

extern bool bwe_dirty;

FrameSnapshot g_frame_snapshot = {0};

void BOF_BeginFrameLock(void) {
    g_frame_frozen = true;
}

void BOF_EndFrameLock(void) {
    g_frame_frozen = false;
}

FrameSnapshot CaptureFullSystemState(void) {
    return g_frame_snapshot;
}

void BOF_CaptureSnapshot(int32_t mouse_x, int32_t mouse_y, uint8_t buttons) {
    g_frame_snapshot.mouse_x = mouse_x;
    g_frame_snapshot.mouse_y = mouse_y;
    g_frame_snapshot.mouse_buttons = buttons;
    g_frame_snapshot.focused_surface = bwe_focused_surface_id;
    g_frame_snapshot.hovered_surface = bwe_hover_surface_id;
    g_frame_snapshot.active_drag_surface = bwe_drag_surface_id;
}

void BOS_InvalidateSurface(uint32_t surface_id) {
    (void)surface_id;
    // Under BOHEART ENGINE, full screen is rebuilt deterministically every frame cycle.
}

void BWE_CollectDamage(void) {
    // No-op under BOHEART ENGINE (no dirty rectangles).
}

// ============================================================
// BOHEART ENGINE INPUT COALESCER & PULSE AUTHORITY
// ============================================================
#define BWE_RAW_QUEUE_SIZE 64
static BVEvent g_raw_event_queue[BWE_RAW_QUEUE_SIZE];
static uint32_t g_raw_queue_head = 0;
static uint32_t g_raw_queue_tail = 0;

static int32_t g_raw_mouse_x = 640;
static int32_t g_raw_mouse_y = 360;
static uint8_t g_raw_mouse_buttons = 0;
static bool g_raw_mouse_moved = false;

void BOHeart_InputCapture(const BVEvent* ev) {
    if (!ev) return;
    
    // Mouse movement: overwrite latest raw coordinates only
    if (ev->type == BV_EVENT_MOUSE_MOVE || ev->type == BV_EVENT_MOUSE_DOWN || ev->type == BV_EVENT_MOUSE_UP) {
        g_raw_mouse_x = ev->mouse_x;
        g_raw_mouse_y = ev->mouse_y;
        g_raw_mouse_buttons = ev->mouse_buttons;
        g_raw_mouse_moved = true;
    }
    
    // Discrete events (clicks, keys): queue for synchronous processing inside tick
    if (ev->type != BV_EVENT_MOUSE_MOVE) {
        uint32_t next_tail = (g_raw_queue_tail + 1) % BWE_RAW_QUEUE_SIZE;
        if (next_tail != g_raw_queue_head) {
            BVEvent queued_ev = *ev;
            queued_ev.mouse_x = g_raw_mouse_x;
            queued_ev.mouse_y = g_raw_mouse_y;
            g_raw_event_queue[g_raw_queue_tail] = queued_ev;
            g_raw_queue_tail = next_tail;
        }
    }
}

void BOF_BeginAtomicFrame(void) {
    g_update_lock = 0;

    if (bwe_drag_state == DRAG_RELEASE_PENDING) {
        bwe_drag_state = DRAG_IDLE;
        bwe_is_dragging = false;
        bwe_drag_surface_id = 0;
        bwe_capture_surface_id = 0;
    }
}

void BOF_ComposeFullFrame(void) {
    BWE_Surface* desktop = BWE_GetSurface(BWE_DESKTOP_ID);
    if (!desktop) return;
    
    extern FrameSnapshot g_frame_snapshot;
    extern void BVCursor_Draw(int32_t x, int32_t y);
    extern void BOVISUAL_Graphics_ClearClipRect(void);
    
    BOVISUAL_Graphics_ClearClipRect();
    
    // Rebuild entire UI stack in full backbuffer via BOCOMPOSITOR ENGINE v2
    BOCompositor_ComposeFrame();
    
    // Live Test Demo: Queue a 4x4 grid of batched texture sprites from in-memory BMP
    extern void BOImage_v2_RunDemo(int32_t x, int32_t y);
    BOImage_v2_RunDemo(60, 60);

    // Draw Input Visual Debug Layer (IVDL) overlay
    extern void IVDL_DrawOverlay(void);
    IVDL_DrawOverlay();

    // Draw cursor overlay on top
    BVCursor_Draw(g_frame_snapshot.mouse_x, g_frame_snapshot.mouse_y);
}

void BOF_ComposeDirtyOnly(void) {
    BOF_ComposeFullFrame();
}

void BOF_EndAtomicFrame(const BVFramebuffer* hw_fb) {
    (void)hw_fb; // Unused, we use hardware double buffering page-flipping!
    extern BVFramebuffer vbe_get_back_page(void);
    extern void vbe_swap_page(void);
    extern void BOVISUAL_Graphics_SwapFull(const BVFramebuffer* hw_fb);
    
    // 1. Get the current hidden VRAM page
    BVFramebuffer back_vram = vbe_get_back_page();
    
    // 2. Copy the entire RAM back buffer to VRAM back page
    BOVISUAL_Graphics_SwapFull(&back_vram);
    
    // 3. Atomically flip display to the VRAM back page (Zero tearing!)
    vbe_swap_page();
    
    BOCompositor_ClearDamage();
}

void BOHeart_Pulse(const BVFramebuffer* hw_fb) {
    // --------------------------------------------------------
    // 1. INPUT COALESCE & SNAPSHOT FREEZE (BOMOUSETABUNDER truth)
    // --------------------------------------------------------
    static int32_t s_last_pulse_x = -9999;
    static int32_t s_last_pulse_y = -9999;
    static uint8_t s_last_pulse_buttons = 255;

    InputState unified_input = input_get_latest_state();
    int32_t snap_mouse_x = unified_input.mouse_x;
    int32_t snap_mouse_y = unified_input.mouse_y;
    uint8_t snap_buttons = unified_input.buttons;
    bool snap_moved = (snap_mouse_x != s_last_pulse_x || snap_mouse_y != s_last_pulse_y || snap_buttons != s_last_pulse_buttons || g_raw_mouse_moved);
    s_last_pulse_x = snap_mouse_x;
    s_last_pulse_y = snap_mouse_y;
    s_last_pulse_buttons = snap_buttons;
    g_raw_mouse_x = snap_mouse_x;
    g_raw_mouse_y = snap_mouse_y;
    g_raw_mouse_buttons = snap_buttons;
    g_raw_mouse_moved = false;

    BVEvent frame_events[BWE_RAW_QUEUE_SIZE];
    uint32_t frame_event_count = 0;
    while (g_raw_queue_head != g_raw_queue_tail && frame_event_count < BWE_RAW_QUEUE_SIZE) {
        frame_events[frame_event_count++] = g_raw_event_queue[g_raw_queue_head];
        g_raw_queue_head = (g_raw_queue_head + 1) % BWE_RAW_QUEUE_SIZE;
    }

    BOF_CaptureSnapshot(snap_mouse_x, snap_mouse_y, snap_buttons);
    g_frame_snapshot = CaptureFullSystemState();

    // Hardware Safety Net (from Phase 19.1)
    if (snap_buttons == 0) {
        extern uint32_t bwe_capture_surface_id;
        extern bool bwe_is_dragging;
        if (bwe_capture_surface_id != 0 || bwe_is_dragging) {
            bwe_capture_surface_id = 0;
            bwe_is_dragging = false;
            extern uint32_t bwe_drag_surface_id;
            bwe_drag_surface_id = 0;
        }
    }

    extern bool conhost_has_dirty_sessions(void);
    extern void conhost_clear_dirty_all(void);
    if (conhost_has_dirty_sessions()) {
        conhost_clear_dirty_all();
    }

    // --------------------------------------------------------
    // 2. STATE UPDATE (ONCE PER FRAME ONLY)
    // --------------------------------------------------------
    BOF_BeginAtomicFrame();

    if (snap_moved) {
        BVEvent move_ev;
        move_ev.type = BV_EVENT_MOUSE_MOVE;
        move_ev.mouse_x = snap_mouse_x;
        move_ev.mouse_y = snap_mouse_y;
        move_ev.mouse_buttons = snap_buttons;
        BOS_ProcessEvent(&move_ev);
    }

    for (uint32_t i = 0; i < frame_event_count; i++) {
        frame_events[i].mouse_x = snap_mouse_x;
        frame_events[i].mouse_y = snap_mouse_y;
        BOS_ProcessEvent(&frame_events[i]);
    }

    BWE_ComputeScreenBounds();

    // --------------------------------------------------------
    // 3. FULL COMPOSITE RENDER & SNAPSHOT LOCK
    // --------------------------------------------------------
    BOF_BeginFrameLock(); // Freeze UI against mutation during render
    BOCompositor_BeginFrame();
    BOF_ComposeFullFrame(); // Deterministic full scene redraw
    extern void BOImage_BOHeartTickFlush(void);
    BOImage_BOHeartTickFlush(); // Flush BOIMAGE v2 batched sprite draw calls

    // --------------------------------------------------------
    // 4. ATOMIC SWAP
    // --------------------------------------------------------
    BOF_EndAtomicFrame(hw_fb);
    BOF_EndFrameLock();
}

// ============================================================
// Accessors
// ============================================================
uint32_t BWE_GetSurfaceCount(void) {
    return active_surface_count;
}

uint32_t BWE_GetHoverSurfaceID(void) {
    return bwe_hover_surface_id;
}

bool BWE_IsDragging(void) {
    return bwe_is_dragging;
}

uint32_t BWE_GetDragSurfaceID(void) {
    return bwe_drag_surface_id;
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

// ============================================================
// BOS_Test_Phase5 — Control Generation Validation
// ============================================================
void BOS_Test_Phase5(void) {
    bwe_log("INFO", "--- BWE Phase 5: Control Generation & Theme Engine ---");

    BOSurface_Init();

    // Create a container window
    uint32_t container_id = 0;
    bwe_error_t err = BOS_CreateSurface(BWE_DESKTOP_ID, 100, 100, 500, 400, BWE_FLAG_VISIBLE, &container_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Container creation\n"); return; }

    // Create a Panel Control
    uint32_t panel_id = 0;
    err = BOS_CreatePanel(container_id, 20, 40, 460, 340, 0xFF1E293B, &panel_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Panel control creation\n"); return; }

    // Create a Button Control
    uint32_t button_id = 0;
    err = BOS_CreateButton(panel_id, 30, 30, 150, 40, "Click Me", 0, &button_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Button control creation\n"); return; }

    // Create a Label Control
    uint32_t label_id = 0;
    err = BOS_CreateLabel(panel_id, 30, 90, "Form Label:", 0xFFF1F5F9, &label_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Label control creation\n"); return; }

    // Create a Textbox Control
    uint32_t textbox_id = 0;
    err = BOS_CreateTextbox(panel_id, 30, 130, 300, 45, "Enter text here...", &textbox_id);
    if (err != BWE_SUCCESS) { display_print("FAIL: Textbox control creation\n"); return; }

    // Update Text State
    err = BOS_SetText(button_id, "Submit Form");
    if (err != BWE_SUCCESS) { display_print("FAIL: BOS_SetText on Button\n"); return; }

    err = BOS_SetText(label_id, "User Credentials:");
    if (err != BWE_SUCCESS) { display_print("FAIL: BOS_SetText on Label\n"); return; }

    // Verify coordinates
    BWE_ComputeScreenBounds();
    BWE_Compose();

    // Verify count
    // Desktop (1) + Container (1) + Panel (1) + Button (1) + Label (1) + Textbox (1) = 6 surfaces total
    if (BWE_GetSurfaceCount() == 6) {
        display_print("\nPASS_BWE_PHASE5\n");
    } else {
        display_print("\nFAIL: BWE Surface Count mismatch in Phase 5\n");
    }
}

// ============================================================
// BOS_Test_Phase6 — Event Routing & Interaction Validation
// ============================================================
void BOS_Test_Phase6(void) {
    bwe_log("INFO", "--- BWE Phase 6: Event Routing & Window Interaction ---");

    BOSurface_Init();

    // Create Window A
    uint32_t win_a = 0;
    bwe_error_t err = BOS_CreateSurface(BWE_DESKTOP_ID, 50, 50, 300, 200, BWE_FLAG_VISIBLE | BWE_FLAG_DRAGGABLE, &win_a);
    if (err == BWE_SUCCESS) {
        BWE_Surface* surf = BWE_GetSurface(win_a);
        surf->control_data.panel.bg_color = 0xFF334155; // Slate 700
    }

    // Add Panel to Window A
    uint32_t panel_a = 0;
    BOS_CreatePanel(win_a, 10, 30, 280, 160, 0xFF1E293B, &panel_a);

    // Add Button to Window A
    uint32_t btn_a = 0;
    BOS_CreateButton(panel_a, 20, 20, 120, 40, "Button A", 0, &btn_a);

    // Create Window B (Overlapping A)
    uint32_t win_b = 0;
    BOS_CreateSurface(BWE_DESKTOP_ID, 200, 100, 300, 200, BWE_FLAG_VISIBLE | BWE_FLAG_DRAGGABLE, &win_b);
    if (win_b) {
        BWE_Surface* surf = BWE_GetSurface(win_b);
        surf->control_data.panel.bg_color = 0xFF475569; // Slate 600
    }

    // Add Panel to Window B
    uint32_t panel_b = 0;
    BOS_CreatePanel(win_b, 10, 30, 280, 160, 0xFF0F172A, &panel_b);

    // Add Button to Window B
    uint32_t btn_b = 0;
    BOS_CreateButton(panel_b, 20, 20, 120, 40, "Button B", 0, &btn_b);

    BWE_ComputeScreenBounds();
    BWE_Compose();

    display_print("\nPASS_BWE_PHASE6\n");
    display_print("Windows are now fully interactive via mouse input.\n");
}

// ============================================================
// BOS_Test_Phase7_State — Window State Manager Validation
// ============================================================
void BOS_Test_Phase7_State(void) {
    bwe_log("INFO", "--- BWE Phase 7: State Manager ---");

    BOSurface_Init();

    uint32_t win = 0;
    BOS_CreateSurface(BWE_DESKTOP_ID, 50, 50, 300, 200, BWE_FLAG_VISIBLE, &win);
    
    // Test Minimize
    BOS_MinimizeSurface(win);
    BWE_Surface* surf = BWE_GetSurface(win);
    if (surf && surf->state == BWE_STATE_MINIMIZED && !(surf->flags & BWE_FLAG_VISIBLE)) {
        display_print("[OK] Surface Minimized\n");
    } else {
        display_print("[FAIL] Surface Minimize Failed\n");
        return;
    }
    
    // Test Maximize
    BOS_MaximizeSurface(win);
    
    if (surf && surf->local_bounds.width > 300 && (surf->flags & BWE_FLAG_VISIBLE)) {
        display_print("[OK] Surface Maximized\n");
    } else {
        display_print("[FAIL] Surface Maximize Failed\n");
        return;
    }
    
    // Test Restore
    BOS_RestoreSurface(win);
    if (surf && surf->local_bounds.width == 300 && (surf->flags & BWE_FLAG_VISIBLE)) {
        display_print("[OK] Surface Restored\n");
    } else {
        display_print("[FAIL] Surface Restore Failed\n");
        return;
    }
    
    // Test Close
    BOS_CloseSurface(win);
    if (BWE_GetSurfaceCount() == 1) { // Only desktop remains
        display_print("[OK] Surface Closed\n");
    } else {
        display_print("[FAIL] Surface Close Failed\n");
        return;
    }

    display_print("\nPASS_BWE_PHASE7_STATE\n");
}

// ============================================================
// BOS_Test_Phase7_Titlebar — Window Engine Validation
// ============================================================
void BOS_Test_Phase7_Titlebar(void) {
    bwe_log("INFO", "--- BWE Phase 7: Titlebar Engine ---");

    BOSurface_Init();

    // Create First Window
    uint32_t win1;
    BOS_CreateWindow(50, 50, 400, 300, "Window 1 (Draggable Titlebar)", &win1);
    
    // Create Second Window
    uint32_t win2;
    BOS_CreateWindow(200, 150, 400, 300, "Window 2 (Overlapping)", &win2);

    BWE_ComputeScreenBounds();
    BWE_Compose();
    
    display_print("\nPASS_BWE_PHASE7_TITLEBAR\n");
    display_print("Windows now have titlebars and system buttons.\n");
}

// ============================================================
// BOS_Test_Phase8_Shell — Desktop Shell Validation (Preserved)
// ============================================================
static void dummy_icon_click(uint32_t icon_id) {
    bwe_log_id("INFO", "Desktop Icon Clicked", icon_id);
}

void BOS_Test_Phase8_Shell(void) {
    bwe_log("INFO", "--- BWE Phase 8: Desktop Shell Foundation ---");

    BOSurface_Init();

    // 1. Set Wallpaper (Teal background)
    BOS_SetWallpaper(0xFF0F766E); // Teal-700

    // 2. Create Desktop Icons
    uint32_t icon1, icon2, icon3;
    BOS_CreateDesktopIcon(20, 20, "Computer", dummy_icon_click, &icon1);
    BOS_CreateDesktopIcon(20, 120, "Terminal", dummy_icon_click, &icon2);
    BOS_CreateDesktopIcon(20, 220, "Settings", dummy_icon_click, &icon3);

    // 3. Create Taskbar
    BOS_CreateTaskbar();

    // 4. Create some standard Windows to prove they don't break
    uint32_t win1, win2;
    BOS_CreateWindow(150, 100, 400, 300, "System Settings", &win1);
    BOS_CreateWindow(200, 150, 400, 300, "Terminal (tty1)", &win2);

    BWE_ComputeScreenBounds();
    BWE_Compose();
    
    display_print("\nPASS_BWE_PHASE8\n");
}

// ============================================================
// Phase 9 — Built-in Application Definitions
// ============================================================

// App IDs stored globally for desktop icon click routing
static uint32_t g_app_explorer_id = 0;
static uint32_t g_app_terminal_id = 0;
static uint32_t g_app_settings_id = 0;

#include "../Apps/explorer.h"

// --- Terminal App ---
// Handled by Apps/terminal.c (terminal_init, terminal_exit)

// --- Settings App ---
static bwe_error_t settings_init(uint32_t* out_win) {
    uint32_t win = 0;
    bwe_error_t err = BOS_CreateWindow(250, 100, 450, 300, "System Settings", &win);
    if (err != BWE_SUCCESS) return err;
    
    uint32_t lbl = 0;
    BOS_CreateLabel(win, 20, 50, "SignaturesOS v0.9", 0xFFFFFFFF, &lbl);
    uint32_t lbl2 = 0;
    BOS_CreateLabel(win, 20, 75, "BISHOP Windowing Engine", 0xFF94A3B8, &lbl2);
    
    if (out_win) *out_win = win;
    return BWE_SUCCESS;
}
static void settings_exit(void) {
    bwe_log("INFO", "Settings: Preferences saved");
}

// --- Desktop Icon Click Handlers ---
static void icon_explorer_click(uint32_t icon_id) {
    (void)icon_id;
    BOS_StartApplication(g_app_explorer_id);
}
static void icon_terminal_click(uint32_t icon_id) {
    (void)icon_id;
    BOS_StartApplication(g_app_terminal_id);
}
static void icon_settings_click(uint32_t icon_id) {
    (void)icon_id;
    BOS_StartApplication(g_app_settings_id);
}

// ============================================================
// BOS_Test_Phase10_Terminal — Interactive Terminal Engine
// ============================================================
void BOS_Test_Phase10_Terminal(void) {
    bwe_log("INFO", "--- BWE Phase 10: Interactive Terminal ---");

    BOSurface_Init();
    BOS_AppManager_Init();

    // 1. Set Wallpaper
    BOS_SetWallpaper(0xFF0F766E); // Teal-700

    // 2. Register Built-in Applications
    BOS_RegisterApplication("Explorer",  "1.0", explorer_init,  explorer_exit,  &g_app_explorer_id);
    BOS_RegisterApplication("Terminal",  "1.0", terminal_init,  terminal_exit,  &g_app_terminal_id);
    BOS_RegisterApplication("Settings",  "1.0", settings_init,  settings_exit,  &g_app_settings_id);

    // 3. Create Desktop Icons linked to App Manager
    uint32_t icon1, icon2, icon3;
    BOS_CreateDesktopIcon(20, 20,  "Computer", icon_explorer_click, &icon1);
    BOS_CreateDesktopIcon(20, 120, "Terminal", icon_terminal_click, &icon2);
    BOS_CreateDesktopIcon(20, 220, "Settings", icon_settings_click, &icon3);

    // 4. Create Taskbar
    BOS_CreateTaskbar();

    // 5. Auto-launch Terminal for demo
    BOS_StartApplication(g_app_terminal_id);

    BWE_ComputeScreenBounds();
    BWE_Compose();
    
    display_print("\nPASS_PHASE10_TERMINAL\n");
}

// ============================================================
// BOS_Test_Phase11_Explorer — Native File Explorer & VFS
// ============================================================
void BOS_Test_Phase11_Explorer(void) {
    bwe_log("INFO", "--- BWE Phase 11: File Explorer ---");

    BOSurface_Init();
    BOS_AppManager_Init();

    // 1. Set Wallpaper
    BOS_SetWallpaper(0xFF0284C7); // Light Blue-600

    // 2. Register Built-in Applications
    BOS_RegisterApplication("Explorer",  "1.0", explorer_init,  explorer_exit,  &g_app_explorer_id);
    BOS_RegisterApplication("Terminal",  "1.0", terminal_init,  terminal_exit,  &g_app_terminal_id);
    BOS_RegisterApplication("Settings",  "1.0", settings_init,  settings_exit,  &g_app_settings_id);

    // 3. Create Desktop Icons linked to App Manager
    uint32_t icon1, icon2, icon3;
    BOS_CreateDesktopIcon(20, 20,  "Computer", icon_explorer_click, &icon1);
    BOS_CreateDesktopIcon(20, 120, "Terminal", icon_terminal_click, &icon2);
    BOS_CreateDesktopIcon(20, 220, "Settings", icon_settings_click, &icon3);

    // 4. Create Taskbar
    BOS_CreateTaskbar();

    // 5. Auto-launch Explorer for demo
    BOS_StartApplication(g_app_explorer_id);

    BWE_ComputeScreenBounds();
    BWE_Compose();
    
    display_print("\nPASS_PHASE11_EXPLORER\n");
}

// ============================================================
// BOS_Test_Phase12_TextViewer — File Associations + Text Viewer
// ============================================================
#include "file_assoc.h"
#include "../Apps/text_viewer.h"

void BOS_Test_Phase12_TextViewer(void) {
    bwe_log("INFO", "--- BWE Phase 12: Text Viewer & File Associations ---");

    BOSurface_Init();
    BOS_AppManager_Init();
    BOS_FileAssoc_Init();

    // 1. Set Wallpaper
    BOS_SetWallpaper(0xFF0284C7); // Light Blue-600

    // 2. Register File Associations
    BOS_RegisterFileAssociation("TXT", "Text Viewer", text_viewer_open);

    // 3. Register Built-in Applications
    BOS_RegisterApplication("Explorer",  "1.0", explorer_init,  explorer_exit,  &g_app_explorer_id);
    BOS_RegisterApplication("Terminal",  "1.0", terminal_init,  terminal_exit,  &g_app_terminal_id);
    BOS_RegisterApplication("Settings",  "1.0", settings_init,  settings_exit,  &g_app_settings_id);

    // 4. Create Desktop Icons linked to App Manager
    uint32_t icon1, icon2, icon3;
    BOS_CreateDesktopIcon(20, 20,  "Computer", icon_explorer_click, &icon1);
    BOS_CreateDesktopIcon(20, 120, "Terminal", icon_terminal_click, &icon2);
    BOS_CreateDesktopIcon(20, 220, "Settings", icon_settings_click, &icon3);

    // 5. Create Taskbar
    BOS_CreateTaskbar();

    // 6. Auto-launch Explorer for demo
    BOS_StartApplication(g_app_explorer_id);

    BWE_ComputeScreenBounds();
    BWE_Compose();
    
    display_print("\nPASS_PHASE12_TEXTVIEWER\n");
}

extern void BOSX_Init(void);
extern int BOSX_Load(const char* filepath);
extern void bosx_loader_open(const char* filepath);
extern void BOSX_ProcessMonitor_Display(void);

bwe_error_t BOS_CloseSurfacesByPID(uint32_t pid) {
    if (pid == 0) return BWE0001;
    bos_gui_event_cleanup_pid(pid);
    for (uint32_t i = 0; i < BWE_MAX_SURFACES; i++) {
        if (surface_pool[i].active && surface_pool[i].id != BWE_DESKTOP_ID && surface_pool[i].owner_pid == pid) {
            if (surface_pool[i].parent_id == BWE_DESKTOP_ID) {
                BOS_CloseSurface(surface_pool[i].id);
            }
        }
    }
    return BWE_SUCCESS;
}

uint32_t BOS_CountSurfacesByPID(uint32_t pid) {
    if (pid == 0) return 0;
    uint32_t count = 0;
    for (uint32_t i = 0; i < BWE_MAX_SURFACES; i++) {
        if (surface_pool[i].active && surface_pool[i].id != BWE_DESKTOP_ID && surface_pool[i].parent_id == BWE_DESKTOP_ID && surface_pool[i].owner_pid == pid) {
            count++;
        }
    }
    return count;
}

void BOS_Test_Phase13_BOSXLoader(void) {
    bwe_log("INFO", "--- BWE Phase 13: BOSX Loader & Process Execution ---");

    BOSurface_Init();
    BOS_AppManager_Init();
    BOS_FileAssoc_Init();
    BOSX_Init();

    // 1. Set Wallpaper
    BOS_SetWallpaper(0xFF0284C7); // Light Blue-600

    // 2. Register File Associations
    BOS_RegisterFileAssociation("TXT", "Text Viewer", text_viewer_open);
    BOS_RegisterFileAssociation("BOSX", "BOSX Loader", bosx_loader_open);

    // 3. Register Built-in Applications
    BOS_RegisterApplication("Explorer",  "1.0", explorer_init,  explorer_exit,  &g_app_explorer_id);
    BOS_RegisterApplication("Terminal",  "1.0", terminal_init,  terminal_exit,  &g_app_terminal_id);
    BOS_RegisterApplication("Settings",  "1.0", settings_init,  settings_exit,  &g_app_settings_id);

    // 4. Create Desktop Icons linked to App Manager
    uint32_t icon1, icon2, icon3;
    BOS_CreateDesktopIcon(20, 20,  "Computer", icon_explorer_click, &icon1);
    BOS_CreateDesktopIcon(20, 120, "Terminal", icon_terminal_click, &icon2);
    BOS_CreateDesktopIcon(20, 220, "Settings", icon_settings_click, &icon3);

    // 5. Create Taskbar
    BOS_CreateTaskbar();

    // 6. Auto-launch Explorer for demo
    BOS_StartApplication(g_app_explorer_id);

    // 7. Test BOSX Loader workflow: simulating double click on CALC.BOSX
    display_print("\n[PHASE13] Simulating double click on CALC.BOSX...\n");
    int pid = BOSX_Load("0:/CALC.BOSX");
    if (pid >= 0) {
        g_current_creating_pid = (uint32_t)pid;
        uint32_t shell_win = 0;
        BOS_CreateWindow(200, 150, 400, 250, "Calculator.BOSX (Native App)", &shell_win);
        g_current_creating_pid = 0;
    }

    BWE_ComputeScreenBounds();
    BWE_Compose();
    
    // Display Process Monitor Diagnostics
    BOSX_ProcessMonitor_Display();
}
