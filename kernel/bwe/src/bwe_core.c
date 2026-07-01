#include "../include/bwe.h"

// External kernel display printing APIs
extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);

// BWE Static Master Pool
BWE_Window g_windows[BWE_MAX_WINDOWS];
static uint16_t   g_window_generations[BWE_MAX_WINDOWS];
static uint32_t   g_active_window_count = 0;

// Event Queue State
static BWE_EventQueue g_event_queue;

// Global Focus State
uint32_t g_active_window_id = BWE_DESKTOP_ID;
uint32_t g_focused_window_id = BWE_DESKTOP_ID;

// Global mouse tracking coordinates
int32_t g_bwe_mouse_x = 0;
int32_t g_bwe_mouse_y = 0;

// External screen resolution variables from ATOMS OS VBE
extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

// ============================================================
// Internal Diagnostic Logging Helpers
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
    display_print(" ID #");
    display_print_dec(id);
    display_print("\n");
}

// ============================================================
// O(1) Stable Window Registry Implementation
// ============================================================

BWE_Window* BWE_GetWindow(uint32_t window_id) {
    if (window_id == BWE_DESKTOP_ID) {
        if (g_windows[0].state != BWE_STATE_DESTROYED && g_windows[0].id == BWE_DESKTOP_ID) {
            return &g_windows[0];
        }
        return 0;
    }

    uint32_t slot = window_id & 0xFF;
    if (slot >= BWE_MAX_WINDOWS) {
        return 0;
    }

    BWE_Window* win = &g_windows[slot];
    if (win->id == window_id && win->state != BWE_STATE_DESTROYED) {
        return win;
    }

    return 0;
}

bool BWE_ValidateWindow(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return false;
    return (win->state != BWE_STATE_DESTROYED);
}

uint32_t BWE_GetWindowCount(void) {
    return g_active_window_count;
}

bwe_error_t BWE_EnumerateWindows(uint32_t* out_ids, uint32_t max_count, uint32_t* out_count) {
    if (!out_ids || !out_count) {
        bwe_log("ERROR", "EnumerateWindows: NULL outputs");
        return BWE0001;
    }

    uint32_t count = 0;
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        if (g_windows[i].state != BWE_STATE_DESTROYED && g_windows[i].id != 0) {
            if (count < max_count) {
                out_ids[count] = g_windows[i].id;
                count++;
            } else {
                break;
            }
        }
    }

    *out_count = count;
    return BWE_SUCCESS;
}

static int32_t find_free_slot(void) {
    for (uint32_t i = 1; i < BWE_MAX_WINDOWS; i++) {
        if (g_windows[i].state == BWE_STATE_DESTROYED) {
            return (int32_t)i;
        }
    }
    return -1;
}

bwe_error_t BWE_AllocateWindowSlot(uint32_t* out_id, uint32_t* out_slot) {
    int32_t slot = find_free_slot();
    if (slot == -1) {
        bwe_log("ERROR", "AllocateWindowSlot: Pool Exhausted");
        return BWE0004;
    }

    uint32_t gen = g_window_generations[slot];
    uint32_t id = (gen << 8) | ((uint32_t)slot & 0xFF);

    *out_id = id;
    *out_slot = (uint32_t)slot;
    return BWE_SUCCESS;
}

void BWE_FreeWindowSlot(uint32_t window_id) {
    uint32_t slot = window_id & 0xFF;
    if (slot < BWE_MAX_WINDOWS) {
        g_window_generations[slot]++;
        if (g_window_generations[slot] == 0) {
            g_window_generations[slot] = 1;
        }
    }
}

// ============================================================
// Core Invalidation & Dirty Rectangle Tracking
// ============================================================

static void invalidate_descendants_recursive(BWE_Window* win) {
    win->is_dirty = true;
    for (uint32_t i = 0; i < win->child_count; i++) {
        BWE_Window* child = BWE_GetWindow(win->children[i]);
        if (child) {
            invalidate_descendants_recursive(child);
        }
    }
}

bwe_error_t BWE_InvalidateWindow(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) {
        bwe_log_id("WARN", "InvalidateWindow: Invalid Window ID", window_id);
        return BWE0001;
    }

    // Recursively invalidate all descendants
    invalidate_descendants_recursive(win);
    
    // Bubble up to parents so they are also recomposed
    uint32_t curr_parent = win->parent_id;
    while (curr_parent != BWE_DESKTOP_ID && curr_parent != win->id) {
        BWE_Window* p = BWE_GetWindow(curr_parent);
        if (p) {
            p->is_dirty = true;
            curr_parent = p->parent_id;
        } else {
            break;
        }
    }

    return BWE_SUCCESS;
}

bwe_error_t BOS_SetBounds(uint32_t window_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) {
        bwe_log_id("ERROR", "SetBounds: Invalid Window ID", window_id);
        return BWE0001;
    }

    if (width < (uint32_t)win->min_size.width) width = win->min_size.width;
    if (height < (uint32_t)win->min_size.height) height = win->min_size.height;
    if (win->max_size.width > 0 && width > (uint32_t)win->max_size.width) width = win->max_size.width;
    if (win->max_size.height > 0 && height > (uint32_t)win->max_size.height) height = win->max_size.height;

    win->old_screen_bounds = win->screen_bounds;

    win->local_bounds.x = (int32_t)x;
    win->local_bounds.y = (int32_t)y;
    win->local_bounds.width = (int32_t)width;
    win->local_bounds.height = (int32_t)height;

    if (win->parent_id == BWE_DESKTOP_ID) {
        win->screen_bounds.x = win->local_bounds.x;
        win->screen_bounds.y = win->local_bounds.y;
    } else {
        BWE_Window* parent = BWE_GetWindow(win->parent_id);
        if (parent) {
            win->screen_bounds.x = parent->screen_bounds.x + win->local_bounds.x;
            win->screen_bounds.y = parent->screen_bounds.y + win->local_bounds.y;
        } else {
            win->screen_bounds.x = win->local_bounds.x;
            win->screen_bounds.y = win->local_bounds.y;
        }
    }
    win->screen_bounds.width = win->local_bounds.width;
    win->screen_bounds.height = win->local_bounds.height;

    // Recursively update all child widgets' screen bounds based on new parent bounds
    extern void BWE_UpdateLayout(uint32_t parent_id);
    BWE_UpdateLayout(window_id);

    BWE_InvalidateWindow(window_id);
    return BWE_SUCCESS;
}

bwe_error_t BOS_GetBounds(uint32_t window_id, BWE_Rect* out_bounds) {
    if (!out_bounds) {
        return BWE0001;
    }

    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) {
        return BWE0001;
    }

    *out_bounds = win->local_bounds;
    return BWE_SUCCESS;
}

// ============================================================
// Event Queue Subsystem Implementation
// ============================================================

bwe_error_t BWE_EventQueue_Push(const BWE_Event* event) {
    if (!event) return BWE0001;

    if (g_event_queue.count >= BWE_EVENT_QUEUE_SIZE) {
        return BWE0006;
    }

    g_event_queue.events[g_event_queue.tail] = *event;
    g_event_queue.tail = (g_event_queue.tail + 1) % BWE_EVENT_QUEUE_SIZE;
    g_event_queue.count++;

    return BWE_SUCCESS;
}

bwe_error_t BWE_EventQueue_Pop(BWE_Event* out_event) {
    if (!out_event) return BWE0001;

    if (g_event_queue.count == 0) {
        return BWE0001;
    }

    *out_event = g_event_queue.events[g_event_queue.head];
    g_event_queue.head = (g_event_queue.head + 1) % BWE_EVENT_QUEUE_SIZE;
    g_event_queue.count--;

    return BWE_SUCCESS;
}

bwe_error_t BWE_EventQueue_Peek(BWE_Event* out_event) {
    if (!out_event) return BWE0001;

    if (g_event_queue.count == 0) {
        return BWE0001;
    }

    *out_event = g_event_queue.events[g_event_queue.head];
    return BWE_SUCCESS;
}

void BWE_EventQueue_Clear(void) {
    g_event_queue.head = 0;
    g_event_queue.tail = 0;
    g_event_queue.count = 0;
}

// ============================================================
// Event Processing Adapter & Compatibility Hub
// ============================================================

void BOS_ProcessEvent(const BVEvent* event) {
    if (!event) return;

    // Translate raw BVEvent into BWE_Event
    BWE_Event bwe_ev;
    bwe_ev.target_id = 0;
    
    switch (event->type) {
        case BV_EVENT_MOUSE_MOVE:
            bwe_ev.type = BWE_EVENT_MOUSE_MOVE;
            bwe_ev.data.mouse.x = event->mouse_x;
            bwe_ev.data.mouse.y = event->mouse_y;
            bwe_ev.data.mouse.buttons = event->mouse_buttons;
            bwe_ev.data.mouse.wheel_delta = 0;
            break;
        case BV_EVENT_MOUSE_DOWN:
            bwe_ev.type = BWE_EVENT_MOUSE_DOWN;
            bwe_ev.data.mouse.x = event->mouse_x;
            bwe_ev.data.mouse.y = event->mouse_y;
            bwe_ev.data.mouse.buttons = event->mouse_buttons;
            bwe_ev.data.mouse.wheel_delta = 0;
            break;
        case BV_EVENT_MOUSE_UP:
            bwe_ev.type = BWE_EVENT_MOUSE_UP;
            bwe_ev.data.mouse.x = event->mouse_x;
            bwe_ev.data.mouse.y = event->mouse_y;
            bwe_ev.data.mouse.buttons = event->mouse_buttons;
            bwe_ev.data.mouse.wheel_delta = 0;
            break;
        case BV_EVENT_KEY_DOWN:
            bwe_ev.type = BWE_EVENT_KEY_DOWN;
            bwe_ev.data.key.key_code = event->key_code;
            bwe_ev.data.key.modifiers = (event->shift ? 1 : 0) | (event->ctrl ? 2 : 0) | (event->alt ? 4 : 0);
            break;
        case BV_EVENT_KEY_UP:
            bwe_ev.type = BWE_EVENT_KEY_UP;
            bwe_ev.data.key.key_code = event->key_code;
            bwe_ev.data.key.modifiers = (event->shift ? 1 : 0) | (event->ctrl ? 2 : 0) | (event->alt ? 4 : 0);
            break;
        default:
            bwe_ev.type = BWE_EVENT_NONE;
            break;
    }

    if (bwe_ev.type != BWE_EVENT_NONE) {
        BWE_EventQueue_Push(&bwe_ev);

        // Process mouse dragging/resizing interaction
        if (bwe_ev.type == BWE_EVENT_MOUSE_MOVE || bwe_ev.type == BWE_EVENT_MOUSE_DOWN || bwe_ev.type == BWE_EVENT_MOUSE_UP) {
            g_bwe_mouse_x = bwe_ev.data.mouse.x;
            g_bwe_mouse_y = bwe_ev.data.mouse.y;

            // 1. Let window manager process dragging/resizing state machine
            BWE_ProcessMouseInteraction(bwe_ev.data.mouse.x, bwe_ev.data.mouse.y, bwe_ev.data.mouse.buttons);

            // 2. Dispatch events to the topmost child window/control under the cursor
            extern bool BWE_IsDraggingActive(void);
            extern bool BWE_IsResizingActive(void);
            if (!BWE_IsDraggingActive() && !BWE_IsResizingActive()) {
                extern uint32_t g_z_order_stack[BWE_MAX_WINDOWS];
                extern uint32_t g_z_stack_count;
                static uint32_t s_hovered_control_id = 0;

                // Recursive helper to find the leaf-most control under coordinates
                extern BWE_Window* BWE_GetWindow(uint32_t window_id);
                BWE_Window* target_win = 0;
                
                for (int32_t i = (int32_t)g_z_stack_count - 1; i >= 0; i--) {
                    uint32_t win_id = g_z_order_stack[i];
                    BWE_HitZone hit = BWE_HitTest(win_id, bwe_ev.data.mouse.x, bwe_ev.data.mouse.y);
                    if (hit != BWE_HIT_NONE && win_id != BWE_DESKTOP_ID) {
                        BWE_Window* top_win = BWE_GetWindow(win_id);
                        if (top_win) {
                            // Find leaf-most child control
                            BWE_Window* curr = top_win;
                            bool found_deeper = true;
                            while (found_deeper) {
                                found_deeper = false;
                                for (int32_t j = (int32_t)curr->child_count - 1; j >= 0; j--) {
                                    BWE_Window* child = BWE_GetWindow(curr->children[j]);
                                    if (child && child->state != BWE_STATE_HIDDEN) {
                                        if (bwe_ev.data.mouse.x >= child->screen_bounds.x &&
                                            bwe_ev.data.mouse.x < child->screen_bounds.x + child->screen_bounds.width &&
                                            bwe_ev.data.mouse.y >= child->screen_bounds.y &&
                                            bwe_ev.data.mouse.y < child->screen_bounds.y + child->screen_bounds.height) {
                                            curr = child;
                                            found_deeper = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            target_win = curr;
                            break;
                        }
                    }
                }

                uint32_t leaf_id = target_win ? target_win->id : BWE_DESKTOP_ID;

                // Hover state tracking (MOUSE_ENTER / MOUSE_LEAVE)
                if (bwe_ev.type == BWE_EVENT_MOUSE_MOVE) {
                    if (leaf_id != s_hovered_control_id) {
                        if (s_hovered_control_id != 0) {
                            BWE_Window* old_hover = BWE_GetWindow(s_hovered_control_id);
                            if (old_hover && old_hover->on_event) {
                                BWE_Event leave_ev;
                                leave_ev.type = BWE_EVENT_MOUSE_LEAVE;
                                leave_ev.target_id = s_hovered_control_id;
                                old_hover->on_event(s_hovered_control_id, &leave_ev);
                            }
                        }
                        if (target_win && target_win->on_event) {
                            BWE_Event enter_ev;
                            enter_ev.type = BWE_EVENT_MOUSE_ENTER;
                            enter_ev.target_id = leaf_id;
                            target_win->on_event(leaf_id, &enter_ev);
                        }
                        s_hovered_control_id = leaf_id;
                    }
                }

                // Focus routing on click
                if (bwe_ev.type == BWE_EVENT_MOUSE_DOWN && target_win) {
                    extern bwe_error_t BOS_SetFocus(uint32_t window_id);
                    BOS_SetFocus(leaf_id);
                }

                // Dispatch mouse event to the target leaf-most window/control
                BWE_Window* dispatch_target = BWE_GetWindow(leaf_id);
                if (dispatch_target && dispatch_target->on_event) {
                    bwe_ev.target_id = leaf_id;
                    dispatch_target->on_event(leaf_id, &bwe_ev);
                }
            }
        } else {
            // Dispatch keyboard events to the currently focused window/widget
            extern uint32_t g_focused_window_id;
            if (g_focused_window_id != BWE_DESKTOP_ID) {
                BWE_Window* target = BWE_GetWindow(g_focused_window_id);
                if (target && target->on_event) {
                    bwe_ev.target_id = g_focused_window_id;
                    target->on_event(g_focused_window_id, &bwe_ev);
                }
            }
        }
    }
}

// BWE_Compose Compatibility Wrapper
void BWE_Compose(void) {
    extern BVFramebuffer* vbe_get_framebuffer(void);
    BWE_ComposeFrame(vbe_get_framebuffer());
}

// ============================================================
// Master Subsystem Initialize
// ============================================================

bwe_error_t BWE_Initialize(void) {
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        g_windows[i].id = 0;
        g_windows[i].state = BWE_STATE_DESTROYED;
        g_window_generations[i] = 1;
    }

    BWE_EventQueue_Clear();

    BWE_Window* desktop = &g_windows[0];
    desktop->id = BWE_DESKTOP_ID;
    desktop->parent_id = BWE_DESKTOP_ID;
    desktop->owner_pid = 0;
    desktop->child_count = 0;
    desktop->z_order = 0;
    desktop->type = BWE_TYPE_DESKTOP;
    desktop->state = BWE_STATE_ACTIVE;

    int32_t scr_w = (g_kernel_screen_width > 0) ? (int32_t)g_kernel_screen_width : 1024;
    int32_t scr_h = (g_kernel_screen_height > 0) ? (int32_t)g_kernel_screen_height : 768;

    desktop->local_bounds.x = 0;
    desktop->local_bounds.y = 0;
    desktop->local_bounds.width = scr_w;
    desktop->local_bounds.height = scr_h;
    desktop->screen_bounds = desktop->local_bounds;
    desktop->restore_bounds = desktop->local_bounds;
    desktop->old_screen_bounds = desktop->local_bounds;

    desktop->flags = BWE_WINDOW_BORDERLESS;
    desktop->opacity = 255;
    desktop->is_dirty = false;
    desktop->user_data = 0;

    g_active_window_count = 1;

    // Register Desktop in Z stack
    extern uint32_t g_z_order_stack[BWE_MAX_WINDOWS];
    extern uint32_t g_z_stack_count;
    g_z_order_stack[0] = BWE_DESKTOP_ID;
    g_z_stack_count = 1;

    bwe_log("INFO", "BOSurface Window Engine V2.0 Initialized successfully");
    bwe_log_id("INFO", "Desktop window active", BWE_DESKTOP_ID);

    return BWE_SUCCESS;
}

// Compatibility Hooks for kernel.c graphical loop
void BOHeart_InputCapture(const BVEvent* event) {
    BOS_ProcessEvent(event);
}

typedef struct {
    int32_t mouse_x;
    int32_t mouse_y;
    uint8_t buttons;
    int32_t scroll;
} BWE_InputState;

extern BWE_InputState input_get_latest_state(void);

void BOHeart_Pulse(const BVFramebuffer* hw_fb) {
    static int32_t s_last_pulse_x = -9999;
    static int32_t s_last_pulse_y = -9999;
    static uint8_t s_last_pulse_buttons = 255;

    BWE_InputState unified_input = input_get_latest_state();
    int32_t snap_mouse_x = unified_input.mouse_x;
    int32_t snap_mouse_y = unified_input.mouse_y;
    uint8_t snap_buttons = unified_input.buttons;

    bool snap_moved = (snap_mouse_x != s_last_pulse_x || snap_mouse_y != s_last_pulse_y || snap_buttons != s_last_pulse_buttons);
    s_last_pulse_x = snap_mouse_x;
    s_last_pulse_y = snap_mouse_y;
    s_last_pulse_buttons = snap_buttons;

    if (snap_moved) {
        BVEvent move_ev;
        move_ev.type = BV_EVENT_MOUSE_MOVE;
        move_ev.mouse_x = snap_mouse_x;
        move_ev.mouse_y = snap_mouse_y;
        move_ev.mouse_buttons = snap_buttons;
        BOS_ProcessEvent(&move_ev);
    }

    BWE_ComposeFrame(hw_fb);
}

void bodebug_dump(void) {
    // Diagnostic telemetry dump stub
}

// BWE V2.0 Core Z-Order Stack definitions
uint32_t g_z_order_stack[BWE_MAX_WINDOWS];
uint32_t g_z_stack_count = 0;

// Legacy BOSurface globals & stubs
uint32_t g_current_creating_pid = 0;

void BOS_CloseSurfacesByPID(uint32_t pid) {
    (void)pid;
}

uint32_t BOS_CountSurfacesByPID(uint32_t pid) {
    (void)pid;
    return 0;
}

void* BOS_GetApplication(uint32_t pid) {
    (void)pid;
    return 0;
}

void* BWE_GetSurface(uint32_t id) {
    (void)id;
    return 0;
}

void BOS_SetText(uint32_t id, const char* text) {
    (void)id;
    (void)text;
}

bool bos_gui_event_pop(uint32_t id, void* ev) {
    (void)id;
    (void)ev;
    return false;
}

uint32_t BWE_GetHoverSurfaceID(void) {
    return 0;
}

bool BWE_IsDragging(void) {
    return false;
}

uint32_t BWE_GetDragSurfaceID(void) {
    return 0;
}

void terminal_init_for_session(void* session) {
    (void)session;
}

void BOF_BeginAtomicFrame(void) {
    // Legacy frame preparation
}

