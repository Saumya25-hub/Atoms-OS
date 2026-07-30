#include "../include/bwe.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/system/boot/boot_mode.h"


// External references
extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);
extern BWE_Window* BWE_GetWindow(uint32_t window_id);
extern bwe_error_t BWE_AllocateWindowSlot(uint32_t* out_id, uint32_t* out_slot);
extern void        BWE_FreeWindowSlot(uint32_t window_id);
extern uint32_t    g_active_window_id;
extern uint32_t    g_focused_window_id;
extern uint32_t    g_z_order_stack[BWE_MAX_WINDOWS];
extern uint32_t    g_z_stack_count;
extern uint32_t    g_kernel_screen_width;
extern uint32_t    g_kernel_screen_height;

// Dragging and Resizing State Registers
static bool        s_is_dragging = false;
static uint32_t    s_drag_win_id = 0;
static int32_t     s_drag_offset_x = 0;
static int32_t     s_drag_offset_y = 0;

static bool        s_is_resizing = false;
static uint32_t    s_resize_win_id = 0;
static BWE_HitZone s_resize_zone = BWE_HIT_NONE;
static int32_t     s_resize_start_x = 0;
static int32_t     s_resize_start_y = 0;
static BWE_Rect    s_resize_start_bounds = {0,0,0,0};

static uint8_t     s_prev_buttons = 0;

// Internal Diagnostic Logging Helpers (defined in bwe_core.c)
extern void bwe_log(const char* level, const char* msg);
extern void bwe_log_id(const char* level, const char* msg, uint32_t id);

// ============================================================
// Tree Ancestry and Validation Operations
// ============================================================

bool BWE_HasAncestor(uint32_t window_id, uint32_t ancestor_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return false;

    uint32_t curr = win->parent_id;
    while (curr != BWE_DESKTOP_ID && curr != window_id) {
        if (curr == ancestor_id) {
            return true;
        }
        BWE_Window* parent = BWE_GetWindow(curr);
        if (parent) {
            curr = parent->parent_id;
        } else {
            break;
        }
    }
    return false;
}

// Internal helper to push a window to Z stack
static void z_stack_push(uint32_t window_id) {
    for (uint32_t i = 0; i < g_z_stack_count; i++) {
        if (g_z_order_stack[i] == window_id) return;
    }
    if (g_z_stack_count < BWE_MAX_WINDOWS) {
        g_z_order_stack[g_z_stack_count++] = window_id;
    }
}

// Internal helper to remove a window from Z stack
static void z_stack_remove(uint32_t window_id) {
    for (uint32_t i = 0; i < g_z_stack_count; i++) {
        if (g_z_order_stack[i] == window_id) {
            for (uint32_t j = i; j < g_z_stack_count - 1; j++) {
                g_z_order_stack[j] = g_z_order_stack[j + 1];
            }
            g_z_stack_count--;
            break;
        }
    }
}

// ============================================================
// Window Lifecycle Operations
// ============================================================

bwe_error_t BOS_CreateSurface(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t flags, uint32_t* out_id) {
    if (!out_id) {
        bwe_log("ERROR", "CreateSurface: NULL ID output pointer");
        return BWE0001;
    }

    BWE_Window* parent = BWE_GetWindow(parent_id);
    if (!parent && parent_id != BWE_DESKTOP_ID) {
        bwe_log_id("ERROR", "CreateSurface: Parent not found", parent_id);
        return BWE0002;
    }

    if (parent_id == BWE_DESKTOP_ID) {
        parent = BWE_GetWindow(BWE_DESKTOP_ID);
        if (!parent) {
            bwe_log("ERROR", "CreateSurface: Desktop root not initialized");
            return BWE0002;
        }
    }

    if (parent->child_count >= BWE_MAX_CHILDREN) {
        bwe_log_id("ERROR", "CreateSurface: Parent children array overflow", parent->id);
        return BWE0004;
    }

    uint32_t id = 0;
    uint32_t slot = 0;
    bwe_error_t err = BWE_AllocateWindowSlot(&id, &slot);
    if (err != BWE_SUCCESS) {
        return err;
    }

    extern BWE_Window g_windows[];
    BWE_Window* win = &g_windows[slot];
    memset(win, 0, sizeof(BWE_Window));

    win->id = id;
    win->parent_id = parent->id;
    extern uint32_t g_current_creating_pid;
    win->owner_pid = g_current_creating_pid;
    win->child_count = 0;
    // Phase 14 Telemetry
    extern void display_print(const char*);
    extern void display_print_dec(uint32_t);
    extern void display_print_hex(uint64_t);
    if (!BOS_IsReleaseMode()) {

        display_print("\n--- PHASE 14 AUTOPSY: Window Created ---\n");
        display_print("Window ID: "); display_print_dec(id); display_print("\n");
        display_print("Parent ID: "); display_print_dec(parent_id); display_print("\n");
        display_print("Width: "); display_print_dec(width); display_print("\n");
        display_print("Height: "); display_print_dec(height); display_print("\n");
        void* ret_addr = __builtin_return_address(0);
        display_print("Return Addr: 0x"); display_print_hex((uint64_t)(uintptr_t)ret_addr); display_print("\n");
        display_print("----------------------------------------\n");
    }


    win->sibling_index = parent->child_count;
    win->type = BWE_TYPE_WINDOW;
    win->state = BWE_STATE_CREATED;

    win->local_bounds.x = (int32_t)x;
    win->local_bounds.y = (int32_t)y;
    win->local_bounds.width = (int32_t)width;
    win->local_bounds.height = (int32_t)height;

    BWE_Rect parent_client;
    extern void BWE_Geometry_CalculateClientBounds(BWE_Window* w, BWE_Rect* o);
    if (parent_id == BWE_DESKTOP_ID) {
        parent_client = parent->screen_bounds;
    } else {
        BWE_Geometry_CalculateClientBounds(parent, &parent_client);
    }
    
    win->screen_bounds.x = parent_client.x + win->local_bounds.x;
    win->screen_bounds.y = parent_client.y + win->local_bounds.y;
    win->screen_bounds.width = win->local_bounds.width;
    win->screen_bounds.height = win->local_bounds.height;
    
    win->restore_bounds = win->screen_bounds;
    win->old_screen_bounds = win->screen_bounds;

    win->anchor_flags = BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP;
    win->margins.left = win->local_bounds.x;
    win->margins.top = win->local_bounds.y;
    win->margins.right = parent_client.width - (win->local_bounds.x + win->local_bounds.width);
    win->margins.bottom = parent_client.height - (win->local_bounds.y + win->local_bounds.height);
    win->baseline_parent_w = parent_client.width;
    win->baseline_parent_h = parent_client.height;

    win->min_size.width = 100; // Sensible minimum width for border buttons
    win->min_size.height = 80;
    win->max_size.width = 0;
    win->max_size.height = 0;

    win->flags = flags;
    win->opacity = 255;
    win->is_dirty = true;
    win->user_data = 0;

    win->control_data.canvas.buffer_w = 0;
    win->control_data.canvas.buffer_h = 0;
    win->control_data.canvas.pixel_buffer = NULL;

    win->on_event = 0;
    win->on_render = 0;

    if (parent->child_count >= BWE_MAX_CHILDREN) {
        bwe_log_id("ERROR", "CreateSurface: Max children exceeded", parent_id);
        return BWE0004;
    }

    win->sibling_index = parent->child_count;
    parent->children[parent->child_count] = id;
    parent->child_count++;

    win->state = BWE_STATE_INITIALIZED;

    if (parent_id == BWE_DESKTOP_ID) {
        z_stack_push(id);
        extern void BWE_UpdateLayout(uint32_t parent_id);
        BWE_UpdateLayout(id);
    } else {
        extern void BWE_UpdateLayout(uint32_t parent_id);
        BWE_UpdateLayout(parent_id);
    }
    BWE_UpdateZOrders();

    extern void BWE_ResetHoverCache(uint32_t window_id);
    BWE_ResetHoverCache(0);

    *out_id = id;
    bwe_log_id("INFO", "Surface created successfully", id);

    return BWE_SUCCESS;
}

static void bwe_validate_hierarchy(uint32_t context_id) {
    extern BWE_Window g_windows[];
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        BWE_Window* win = &g_windows[i];
        if (win->state == BWE_STATE_DESTROYED || win->id == 0) continue;
        
        if (win->child_count > BWE_MAX_CHILDREN) {
            bwe_log_id("ASSERT", "child_count exceeds MAX_CHILDREN", win->id);
            continue;
        }
        
        for (uint32_t j = 0; j < win->child_count; j++) {
            uint32_t cid = win->children[j];
            BWE_Window* child = BWE_GetWindow(cid);
            
            if (!child) {
                bwe_log_id("ASSERT", "Invalid/Destroyed child in children array", win->id);
                continue;
            }
            if (child->parent_id != win->id) {
                bwe_log_id("ASSERT", "Child parent_id mismatch (Orphan)", child->id);
            }
            if (child->sibling_index != j) {
                bwe_log_id("ASSERT", "Sibling index mismatch", child->id);
            }
            for (uint32_t k = j + 1; k < win->child_count; k++) {
                if (win->children[k] == cid) {
                    bwe_log_id("ASSERT", "Duplicate child ID found", win->id);
                }
            }
        }
    }
}

bwe_error_t BOS_DestroySurface(uint32_t window_id) {
    if (window_id == BWE_DESKTOP_ID) {
        bwe_log("ERROR", "DestroySurface: Cannot destroy root desktop");
        return BWE0001;
    }

    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) {
        bwe_log_id("ERROR", "DestroySurface: Window not found", window_id);
        return BWE0001;
    }

    while (win->child_count > 0) {
        uint32_t last_count = win->child_count;
        uint32_t child_id = win->children[0];
        
        BOS_DestroySurface(child_id);
        
        if (win->child_count == last_count) {
            bwe_log_id("ERROR", "DestroySurface: Forcing removal of stale child", child_id);
            for (uint32_t i = 0; i < win->child_count - 1; i++) {
                win->children[i] = win->children[i + 1];
                BWE_Window* sib = BWE_GetWindow(win->children[i]);
                if (sib) sib->sibling_index = i;
            }
            win->child_count--;
        }
    }

    if (g_focused_window_id == window_id) {
        BOS_ClearFocus();
    }
    if (g_active_window_id == window_id) {
        g_active_window_id = BWE_DESKTOP_ID;
    }

    // Cancel dragging or resizing if the target window is destroyed
    if (s_drag_win_id == window_id) {
        s_is_dragging = false;
        s_drag_win_id = 0;
    }
    if (s_resize_win_id == window_id) {
        s_is_resizing = false;
        s_resize_win_id = 0;
        s_resize_zone = BWE_HIT_NONE;
    }

    BWE_Window* parent = BWE_GetWindow(win->parent_id);
    if (parent) {
        int32_t real_index = -1;
        for (uint32_t i = 0; i < parent->child_count; i++) {
            if (parent->children[i] == window_id) {
                real_index = i;
                break;
            }
        }
        
        if (real_index != -1) {
            for (uint32_t i = real_index; i < parent->child_count - 1; i++) {
                parent->children[i] = parent->children[i + 1];
                BWE_Window* sib = BWE_GetWindow(parent->children[i]);
                if (sib) {
                    sib->sibling_index = i;
                }
            }
            parent->child_count--;
        } else {
            bwe_log_id("WARN", "DestroySurface: Child not found in parent array", window_id);
        }
    }

    if (win->parent_id == BWE_DESKTOP_ID) {
        z_stack_remove(window_id);
        if (win->user_data && win->type != BWE_TYPE_DESKTOP_ICON && (uintptr_t)win->user_data > 4096) {
            bwe_log_id("INFO", "SETTINGS_TRACE CLOSE window_id", window_id);
            bwe_log_id("INFO", "SETTINGS_TRACE FREE context address", (uint32_t)(uintptr_t)win->user_data);
            extern void kfree(void* ptr);
            kfree(win->user_data);
            win->user_data = 0;
        }
    }

    if (win->type == BWE_TYPE_CANVAS && win->control_data.canvas.pixel_buffer) {
        extern void kfree(void* ptr);
        kfree(win->control_data.canvas.pixel_buffer);
        win->control_data.canvas.pixel_buffer = NULL;
        win->control_data.canvas.buffer_w = 0;
        win->control_data.canvas.buffer_h = 0;
    }

    win->child_count = 0;
    win->sibling_index = 0;
    win->parent_id = 0;
    for (uint32_t i = 0; i < BWE_MAX_CHILDREN; i++) win->children[i] = 0;

    win->state = BWE_STATE_DESTROYED;
    win->id = 0;
    BWE_FreeWindowSlot(window_id);

    extern void BSCE_Pool_FreeSlot(uint32_t surface_id);
    BSCE_Pool_FreeSlot(window_id);

    extern void BWE_ResetHoverCache(uint32_t window_id);
    BWE_ResetHoverCache(0);

    bwe_validate_hierarchy(window_id);
    
    BWE_UpdateZOrders();
    bwe_log_id("INFO", "Surface destroyed successfully", window_id);

    extern void TaskPanel_Update(void);
    TaskPanel_Update();

    return BWE_SUCCESS;
}

bwe_error_t BOS_CreateWindow(int32_t x, int32_t y, int32_t width, int32_t height, const char* title, uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(BWE_DESKTOP_ID, x, y, width, height, BWE_WINDOW_RESIZABLE | BWE_WINDOW_MOVABLE, &id);
    if (err != BWE_SUCCESS) {
        return err;
    }

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_WINDOW;
        if (title) {
            strncpy(win->control_data.button.text, title, sizeof(win->control_data.button.text) - 1);
            win->control_data.button.text[sizeof(win->control_data.button.text) - 1] = '\0';
        } else {
            win->control_data.button.text[0] = '\0';
        }
        
        // Phase 14 Telemetry (Title)
        extern void display_print(const char*);
        display_print("Window Title Set: ");
        display_print(win->control_data.button.text);
        display_print("\n----------------------------------------\n");

        // Clear canvas fields that overlapped with button.text inside the control_data union
        win->control_data.canvas.on_paint_canvas = NULL;
        win->control_data.canvas.pixel_buffer = NULL;
        win->control_data.canvas.buffer_w = 0;
        win->control_data.canvas.buffer_h = 0;
    }

    if (out_id) {
        *out_id = id;
    }

    extern void TaskPanel_Update(void);
    TaskPanel_Update();

    return BWE_SUCCESS;
}

bwe_error_t BOS_Show(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return BWE0001;

    if (win->state == BWE_STATE_DESTROYED) return BWE0001;

    win->state = BWE_STATE_SHOWN;
    BWE_InvalidateWindow(window_id);
    extern void TaskPanel_Update(void);
    TaskPanel_Update();
    return BWE_SUCCESS;
}

bwe_error_t BOS_Hide(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return BWE0001;

    if (win->state == BWE_STATE_DESTROYED) return BWE0001;

    win->state = BWE_STATE_HIDDEN;
    BWE_InvalidateWindow(window_id);
    extern void TaskPanel_Update(void);
    TaskPanel_Update();
    return BWE_SUCCESS;
}

// ============================================================
// Focus Manager Subsystem
// ============================================================

bwe_error_t BOS_SetFocus(uint32_t window_id) {
    if (g_focused_window_id == window_id) {
        return BWE_SUCCESS;
    }

    BWE_Window* target = BWE_GetWindow(window_id);
    if (!target) {
        return BWE0001;
    }

    if (target->state == BWE_STATE_DESTROYED || target->state == BWE_STATE_HIDDEN) {
        return BWE0005;
    }

    if (g_focused_window_id != BWE_DESKTOP_ID) {
        BWE_Window* old = BWE_GetWindow(g_focused_window_id);
        if (old) {
            old->state = BWE_STATE_DEACTIVATED;
            BWE_InvalidateWindow(old->id);
            
            BWE_Event ev;
            ev.type = BWE_EVENT_FOCUS_LOSS;
            ev.target_id = old->id;
            if (old->on_event) old->on_event(old->id, &ev);
        }
    }

    g_focused_window_id = window_id;
    g_active_window_id = window_id;

    target->state = BWE_STATE_ACTIVE;
    BWE_InvalidateWindow(window_id);

    BWE_Event ev;
    ev.type = BWE_EVENT_FOCUS_GAIN;
    ev.target_id = window_id;
    if (target->on_event) target->on_event(window_id, &ev);

    BWE_BringToFront(window_id);

    extern void TaskPanel_Update(void);
    TaskPanel_Update();

    return BWE_SUCCESS;
}

uint32_t BOS_GetFocus(void) {
    return g_focused_window_id;
}

bwe_error_t BOS_ClearFocus(void) {
    if (g_focused_window_id == BWE_DESKTOP_ID) {
        return BWE_SUCCESS;
    }

    BWE_Window* old = BWE_GetWindow(g_focused_window_id);
    if (old) {
        old->state = BWE_STATE_DEACTIVATED;
        BWE_InvalidateWindow(old->id);
        
        BWE_Event ev;
        ev.type = BWE_EVENT_FOCUS_LOSS;
        ev.target_id = old->id;
        if (old->on_event) old->on_event(old->id, &ev);
    }

    g_focused_window_id = BWE_DESKTOP_ID;
    g_active_window_id = BWE_DESKTOP_ID;
    return BWE_SUCCESS;
}

uint32_t BWE_GetActiveWindow(void) {
    return g_active_window_id;
}

// ============================================================
// Z-Order Manager Subsystem
// ============================================================

static uint32_t get_layer_group(BWE_Window* win) {
    if (win->id == BWE_DESKTOP_ID) return 0;
    if (win->flags & BWE_WINDOW_TOPMOST) return 2;
    if (win->flags & BWE_WINDOW_MODAL) return 3;
    
    return 1; // Normal Standard Windows
}

bwe_error_t BWE_BringToFront(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return BWE0001;

    if (win->parent_id != BWE_DESKTOP_ID) {
        // Bring child to front of parent's children array ONLY if it's a sub-window (e.g. modal/dialog), never for normal controls/background panels
        if (win->type == BWE_TYPE_WINDOW) {
            BWE_Window* parent = BWE_GetWindow(win->parent_id);
            if (parent) {
                int32_t idx = -1;
                for (uint32_t i = 0; i < parent->child_count; i++) {
                    if (parent->children[i] == window_id) {
                        idx = (int32_t)i;
                        break;
                    }
                }
                if (idx != -1 && (uint32_t)idx < parent->child_count - 1) {
                    for (uint32_t i = (uint32_t)idx; i < parent->child_count - 1; i++) {
                        parent->children[i] = parent->children[i + 1];
                        BWE_Window* child = BWE_GetWindow(parent->children[i]);
                        if (child) child->sibling_index = i;
                    }
                    parent->children[parent->child_count - 1] = window_id;
                    win->sibling_index = parent->child_count - 1;
                }
            }
        }
        
        // Also recursively bubble up Z-order update to the top-level parent window
        uint32_t top_id = win->parent_id;
        BWE_Window* curr = BWE_GetWindow(top_id);
        while (curr && curr->parent_id != BWE_DESKTOP_ID && curr->parent_id != curr->id) {
            top_id = curr->parent_id;
            curr = BWE_GetWindow(top_id);
        }
        if (top_id != BWE_DESKTOP_ID) {
            BWE_BringToFront(top_id);
        }
        return BWE_SUCCESS;
    }

    z_stack_remove(window_id);
    z_stack_push(window_id);
    BWE_UpdateZOrders();

    return BWE_SUCCESS;
}

bwe_error_t BWE_SendToBack(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return BWE0001;

    z_stack_remove(window_id);
    for (int32_t i = (int32_t)g_z_stack_count; i > 0; i--) {
        g_z_order_stack[i] = g_z_order_stack[i - 1];
    }
    g_z_order_stack[0] = window_id;
    g_z_stack_count++;

    BWE_UpdateZOrders();
    return BWE_SUCCESS;
}

extern uint32_t g_z_order_version;

bwe_error_t BWE_UpdateZOrders(void) {
    g_z_order_version++;
    if (g_z_stack_count <= 1) {
        for (uint32_t i = 0; i < g_z_stack_count; i++) {
            BWE_Window* w = BWE_GetWindow(g_z_order_stack[i]);
            if (w) w->z_order = i;
        }
        return BWE_SUCCESS;
    }

    for (uint32_t i = 0; i < g_z_stack_count - 1; i++) {
        for (uint32_t j = 0; j < g_z_stack_count - i - 1; j++) {
            BWE_Window* w1 = BWE_GetWindow(g_z_order_stack[j]);
            BWE_Window* w2 = BWE_GetWindow(g_z_order_stack[j + 1]);
            if (w1 && w2) {
                uint32_t g1 = get_layer_group(w1);
                uint32_t g2 = get_layer_group(w2);
                if (g1 > g2) {
                    uint32_t temp = g_z_order_stack[j];
                    g_z_order_stack[j] = g_z_order_stack[j + 1];
                    g_z_order_stack[j + 1] = temp;
                }
            }
        }
    }

    for (uint32_t i = 0; i < g_z_stack_count; i++) {
        BWE_Window* w = BWE_GetWindow(g_z_order_stack[i]);
        if (w) {
            w->z_order = i;
        }
    }
    return BWE_SUCCESS;
}

// ============================================================
// Hit Testing Subsystem Implementation
// ============================================================

BWE_HitZone BWE_HitTest(uint32_t window_id, int32_t screen_x, int32_t screen_y) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win || win->state == BWE_STATE_HIDDEN) return BWE_HIT_NONE;

    BWE_Rect* b = &win->screen_bounds;
    
    // Bounds verify
    if (screen_x < b->x || screen_x >= b->x + b->width ||
        screen_y < b->y || screen_y >= b->y + b->height) {
        return BWE_HIT_NONE;
    }

    // Verify against ancestor client bounds (clipping)
    uint32_t curr_parent = win->parent_id;
    extern void BWE_Geometry_CalculateClientBounds(BWE_Window* w, BWE_Rect* o);
    while (curr_parent != BWE_DESKTOP_ID && curr_parent != 0) {
        BWE_Window* p = BWE_GetWindow(curr_parent);
        if (!p) break;
        BWE_Rect p_client;
        BWE_Geometry_CalculateClientBounds(p, &p_client);
        if (screen_x < p_client.x || screen_x >= p_client.x + p_client.width ||
            screen_y < p_client.y || screen_y >= p_client.y + p_client.height) {
            return BWE_HIT_NONE; // Clipped by parent
        }
        curr_parent = p->parent_id;
    }

    // Borderless windows hit test directly inside client area
    if (win->flags & BWE_WINDOW_BORDERLESS) {
        return BWE_HIT_CLIENT;
    }

    // 1. 8-Way Active Resize Corners hit check (12x12 corner zone pixels)
    int32_t corner = 12;
    if (screen_x < b->x + corner && screen_y < b->y + corner) return BWE_HIT_CORNER_TL;
    if (screen_x >= b->x + b->width - corner && screen_y < b->y + corner) return BWE_HIT_CORNER_TR;
    if (screen_x < b->x + corner && screen_y >= b->y + b->height - corner) return BWE_HIT_CORNER_BL;
    if (screen_x >= b->x + b->width - corner && screen_y >= b->y + b->height - corner) return BWE_HIT_CORNER_BR;

    // 2. 8-Way Resize Borders hit check (5px outer band)
    int32_t border = 5;
    if (screen_y < b->y + border) return BWE_HIT_BORDER_T;
    if (screen_y >= b->y + b->height - border) return BWE_HIT_BORDER_B;
    if (screen_x < b->x + border) return BWE_HIT_BORDER_L;
    if (screen_x >= b->x + b->width - border) return BWE_HIT_BORDER_R;

    // 3. Titlebar Buttons & Body check (y range 5px to 35px)
    int32_t tx = b->x + 5;
    int32_t ty = b->y + 5;
    int32_t tw = b->width - 10;
    
    if (screen_y >= ty && screen_y < ty + 30) {
        // Close Button
        int32_t close_x = tx + tw - 26;
        if (screen_x >= close_x && screen_x < close_x + 20 && screen_y >= ty + 4 && screen_y < ty + 24) {
            return BWE_HIT_CLOSE;
        }
        // Maximize Button
        bool resizable = (win->flags & BWE_WINDOW_RESIZABLE) != 0;
        if (resizable) {
            int32_t max_x = tx + tw - 48;
            if (screen_x >= max_x && screen_x < max_x + 20 && screen_y >= ty + 4 && screen_y < ty + 24) {
                return BWE_HIT_MAX;
            }
        }
        // Minimize Button
        int32_t min_x = tx + tw - (resizable ? 70 : 48);
        if (screen_x >= min_x && screen_x < min_x + 20 && screen_y >= ty + 4 && screen_y < ty + 24) {
            return BWE_HIT_MIN;
        }

        // Titlebar Body
        if (screen_x >= tx && screen_x < tx + tw) {
            return BWE_HIT_TITLEBAR;
        }
    }

    // Default client area hit
    return BWE_HIT_CLIENT;
}

// ============================================================
// Drag & 8-Way Resize Engine Implementation
// ============================================================

void BWE_ProcessMouseInteraction(int32_t mouse_x, int32_t mouse_y, uint8_t buttons, uint32_t event_type) {
    bool down = (event_type == BWE_EVENT_MOUSE_DOWN) || (buttons != 0 && s_prev_buttons == 0);
    bool up = (event_type == BWE_EVENT_MOUSE_UP) || (buttons == 0 && s_prev_buttons != 0);

    if (down) {
        // Find topmost window under mouse
        for (int32_t i = (int32_t)g_z_stack_count - 1; i >= 0; i--) {
            uint32_t win_id = g_z_order_stack[i];
            BWE_HitZone hit = BWE_HitTest(win_id, mouse_x, mouse_y);
            if (hit != BWE_HIT_NONE && win_id != BWE_DESKTOP_ID) {
                // Focus window clicked
                BOS_SetFocus(win_id);
                BWE_Window* win = BWE_GetWindow(win_id);
                if (!win) break;

                // Handle Close
                if (hit == BWE_HIT_CLOSE) {
                    BOS_DestroySurface(win_id);
                    break;
                }
                
                // Handle Maximize
                if (hit == BWE_HIT_MAX) {
                    extern void BWE_WindowMaximize(uint32_t);
                    extern void BWE_WindowRestore(uint32_t);
                    extern bool BWE_WindowIsMaximized(uint32_t);
                    if (BWE_WindowIsMaximized(win_id)) {
                        BWE_WindowRestore(win_id);
                    } else {
                        BWE_WindowMaximize(win_id);
                    }
                    break;
                }

                // Handle drag start
                if (hit == BWE_HIT_TITLEBAR && (win->flags & BWE_WINDOW_MOVABLE)) {
                    s_is_dragging = true;
                    s_drag_win_id = win_id;
                    s_drag_offset_x = mouse_x - win->screen_bounds.x;
                    s_drag_offset_y = mouse_y - win->screen_bounds.y;
                    break;
                }

                // Handle resize start
                if ((hit >= BWE_HIT_BORDER_T && hit <= BWE_HIT_CORNER_BR) && (win->flags & BWE_WINDOW_RESIZABLE)) {
                    s_is_resizing = true;
                    s_resize_win_id = win_id;
                    s_resize_zone = hit;
                    s_resize_start_x = mouse_x;
                    s_resize_start_y = mouse_y;
                    s_resize_start_bounds = win->local_bounds;
                    break;
                }
                break; // Topmost window handles click
            }
        }
    } else if (up) {
        s_is_dragging = false;
        s_drag_win_id = 0;

        s_is_resizing = false;
        s_resize_win_id = 0;
        s_resize_zone = BWE_HIT_NONE;
    } else {
        // Mouse Move
        if (s_is_dragging) {
            BWE_Window* win = BWE_GetWindow(s_drag_win_id);
            if (win) {
                int32_t nx = mouse_x - s_drag_offset_x;
                int32_t ny = mouse_y - s_drag_offset_y;

                // Screen clamping (prevent title bar from dragging completely offscreen)
                if (nx < -win->screen_bounds.width + 50) nx = -win->screen_bounds.width + 50;
                if (nx > (int32_t)g_kernel_screen_width - 50) nx = (int32_t)g_kernel_screen_width - 50;
                if (ny < 0) ny = 0;
                if (ny > (int32_t)g_kernel_screen_height - 30) ny = (int32_t)g_kernel_screen_height - 30;

                BOS_SetBounds(s_drag_win_id, (uint32_t)nx, (uint32_t)ny, (uint32_t)win->local_bounds.width, (uint32_t)win->local_bounds.height);
            }
        } else if (s_is_resizing) {
            BWE_Window* win = BWE_GetWindow(s_resize_win_id);
            if (win) {
                int32_t dx = mouse_x - s_resize_start_x;
                int32_t dy = mouse_y - s_resize_start_y;
                BWE_Rect nb = s_resize_start_bounds;

                // 8-Way Resizing geometry modification math
                if (s_resize_zone == BWE_HIT_BORDER_R || s_resize_zone == BWE_HIT_CORNER_TR || s_resize_zone == BWE_HIT_CORNER_BR) {
                    nb.width += dx;
                }
                if (s_resize_zone == BWE_HIT_BORDER_L || s_resize_zone == BWE_HIT_CORNER_TL || s_resize_zone == BWE_HIT_CORNER_BL) {
                    nb.x += dx;
                    nb.width -= dx;
                }
                if (s_resize_zone == BWE_HIT_BORDER_B || s_resize_zone == BWE_HIT_CORNER_BL || s_resize_zone == BWE_HIT_CORNER_BR) {
                    nb.height += dy;
                }
                if (s_resize_zone == BWE_HIT_BORDER_T || s_resize_zone == BWE_HIT_CORNER_TL || s_resize_zone == BWE_HIT_CORNER_TR) {
                    nb.y += dy;
                    nb.height -= dy;
                }

                // Minimum bounds constraint checks
                if (nb.width >= win->min_size.width && nb.height >= win->min_size.height) {
                    BOS_SetBounds(s_resize_win_id, (uint32_t)nb.x, (uint32_t)nb.y, (uint32_t)nb.width, (uint32_t)nb.height);
                }
            }
        }
    }

    s_prev_buttons = buttons;
}

bool BWE_IsDraggingActive(void) {
    return s_is_dragging;
}

bool BWE_IsResizingActive(void) {
    return s_is_resizing;
}


uint32_t g_forensic_idx_first = 0;
uint32_t g_forensic_idx_mid = 0;
uint32_t g_forensic_idx_last = 0;
bool g_forensic_do_trace = false;

static void bos_canvas_render(BWE_Window* win) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb || !win->control_data.canvas.pixel_buffer) return;

    uint32_t bw = win->control_data.canvas.buffer_w;
    uint32_t bh = win->control_data.canvas.buffer_h;
    const uint32_t* src = win->control_data.canvas.pixel_buffer;

    extern void BWE_Geometry_CalculateClientBounds(BWE_Window* w, BWE_Rect* o);
    BWE_Rect client_bounds;
    BWE_Geometry_CalculateClientBounds(win, &client_bounds);

    int32_t start_x = client_bounds.x;
    int32_t start_y = client_bounds.y;

    if (bw > (uint32_t)client_bounds.width) bw = (uint32_t)client_bounds.width;
    if (bh > (uint32_t)client_bounds.height) bh = (uint32_t)client_bounds.height;

    extern bool BWE_GetClip(BWE_Rect* out_rect);
    BWE_Rect clip;
    bool has_clip = BWE_GetClip(&clip);

    extern void display_print(const char*);
    extern void display_print_dec(uint32_t);
    extern void display_print_hex(uint64_t);

    static int trace_render = 0;
    bool local_trace = false;

    if (bw == 640 || bw == 320) {
        trace_render++;
        if (trace_render == 1) {
            local_trace = true;
            g_forensic_do_trace = true;
            display_print("\n--- PHASE 12 BOS_CANVAS_RENDER FORENSIC ---\n");
            display_print("Source Pointer: 0x"); display_print_hex((uint32_t)(uintptr_t)src); display_print("\n");
            display_print("Destination Pointer (RAM FB): 0x"); display_print_hex((uint32_t)(uintptr_t)fb->buffer); display_print("\n");
            display_print("Width: "); display_print_dec(bw); display_print("\n");
            display_print("Height: "); display_print_dec(bh); display_print("\n");
            display_print("Pitch: "); display_print_dec(fb->pitch); display_print("\n");
            if (has_clip) {
                display_print("Clip Rect: X="); display_print_dec(clip.x);
                display_print(" Y="); display_print_dec(clip.y);
                display_print(" W="); display_print_dec(clip.width);
                display_print(" H="); display_print_dec(clip.height);
                display_print("\n");
            } else {
                display_print("Clip Rect: NONE\n");
            }
            
            g_forensic_idx_first = start_y * fb->width + start_x;
            g_forensic_idx_mid = (start_y + (bh/2)) * fb->width + (start_x + (bw/2));
            g_forensic_idx_last = (start_y + bh - 1) * fb->width + (start_x + bw - 1);
            
            display_print("Before copy - Destination first pixel: 0x"); 
            display_print_hex(((uint32_t*)fb->buffer)[g_forensic_idx_first]); display_print("\n");
        }
    }

    for (uint32_t y = 0; y < bh; y++) {
        for (uint32_t x = 0; x < bw; x++) {
            int32_t sx = start_x + (int32_t)x;
            int32_t sy = start_y + (int32_t)y;

            if (sx >= 0 && sx < (int32_t)fb->width && sy >= 0 && sy < (int32_t)fb->height) {
                if (has_clip) {
                    if (sx >= clip.x && sx < clip.x + clip.width && sy >= clip.y && sy < clip.y + clip.height) {
                        fb->buffer[sy * fb->width + sx] = src[y * bw + x];
                    }
                } else {
                    fb->buffer[sy * fb->width + sx] = src[y * bw + x];
                }
            }
        }
    }

    if (local_trace) {
        display_print("After copy - Destination first pixel: 0x"); display_print_hex(((uint32_t*)fb->buffer)[g_forensic_idx_first]); display_print("\n");
        display_print("After copy - Destination middle pixel: 0x"); display_print_hex(((uint32_t*)fb->buffer)[g_forensic_idx_mid]); display_print("\n");
        display_print("After copy - Destination last pixel: 0x"); display_print_hex(((uint32_t*)fb->buffer)[g_forensic_idx_last]); display_print("\n");
    }
}

bwe_error_t BOS_SurfacePresent(uint32_t window_id, const uint32_t* pixels, uint32_t w, uint32_t h) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return BWE0001;

    extern void* kmalloc(uint32_t size);
    extern void  kfree(void* ptr);

    if (win->control_data.canvas.buffer_w == 0 && win->control_data.canvas.buffer_h == 0) {
        win->control_data.canvas.pixel_buffer = NULL;
    }

    if (win->control_data.canvas.buffer_w != w || win->control_data.canvas.buffer_h != h || !win->control_data.canvas.pixel_buffer) {
        if (win->control_data.canvas.pixel_buffer) {
            kfree(win->control_data.canvas.pixel_buffer);
        }
        win->control_data.canvas.pixel_buffer = (uint32_t*)kmalloc(w * h * 4);
        win->control_data.canvas.buffer_w = w;
        win->control_data.canvas.buffer_h = h;
    }

    if (!win->control_data.canvas.pixel_buffer) {
        return BWE0004; // ERROR ALLOC
    }

    uint32_t* dst = win->control_data.canvas.pixel_buffer;
    for (uint32_t i = 0; i < w * h; i++) {
        dst[i] = pixels[i];
    }

    if (w == 640 || w == 320) {
        static uint32_t trace_frame = 0;
        trace_frame++;
        if (trace_frame == 1 && !BOS_IsReleaseMode()) {

            extern void display_print(const char*);
            extern void display_print_dec(uint32_t);
            extern void display_print_hex(uint64_t);
            display_print("\n--- PHASE 13 AUTOPSY: BOS_SurfacePresent ---\n");
            display_print("Window ID: "); display_print_dec(window_id); display_print("\n");
            display_print("Window Pointer: 0x"); display_print_hex((uint64_t)(uintptr_t)win); display_print("\n");
            display_print("Canvas Pointer: 0x"); display_print_hex((uint64_t)(uintptr_t)dst); display_print("\n");
            display_print("Width: "); display_print_dec(w); display_print("\n");
            display_print("Height: "); display_print_dec(h); display_print("\n");
        }
    }

    win->on_render = bos_canvas_render;
    win->is_dirty = true;

    return BWE_SUCCESS;
}
