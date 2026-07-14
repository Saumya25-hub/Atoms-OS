#include "../include/bwe.h"
#include "kernel/debug/step14_telemetry.h"
#include "kernel/graphics/BSPE/include/bspe.h"
#include "kernel/display/agdae/agdae.h"
#include "kernel/display/bdce/include/bdce_authority.h"
#include "kernel/display/bdce/include/bdce_context.h"

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

uint32_t g_z_order_version = 0;

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

    uint32_t slot = window_id & BWE_WINDOW_SLOT_MASK;
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
    uint32_t id = (gen << BWE_WINDOW_GEN_SHIFT) | ((uint32_t)slot & BWE_WINDOW_SLOT_MASK);

    *out_id = id;
    *out_slot = (uint32_t)slot;
    return BWE_SUCCESS;
}

void BWE_FreeWindowSlot(uint32_t window_id) {
    uint32_t slot = window_id & BWE_WINDOW_SLOT_MASK;
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
    extern void display_print(const char*);
    display_print("[INPUT TRACE] BOS_ProcessEvent\n");

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
    }
}

volatile uint64_t g_bwe_update_calls_count = 0;

void BWE_PumpEvents(void) {
    extern uint64_t timer_get_ticks(void);
    uint64_t pump_start = timer_get_ticks();
    /* STEP 14 */ uint64_t pump_start_tsc = step14_rdtsc(); /* END STEP 14 */
    /* STEP 16 */ step14_log_pump_start(g_step14_telemetry.last_irq_timestamp_ms); /* END STEP 16 */
    
    BWE_Event bwe_ev;
    while (BWE_EventQueue_Pop(&bwe_ev) == BWE_SUCCESS) {
        extern void display_print(const char*);
        display_print("[INPUT TRACE] Queue Pop\n");
        // Process mouse dragging/resizing interaction
        if (bwe_ev.type == BWE_EVENT_MOUSE_MOVE || bwe_ev.type == BWE_EVENT_MOUSE_DOWN || bwe_ev.type == BWE_EVENT_MOUSE_UP) {
            g_bwe_update_calls_count++;
            g_bwe_mouse_x = bwe_ev.data.mouse.x;
            g_bwe_mouse_y = bwe_ev.data.mouse.y;
            extern void display_print(const char*);
            // display_print("(6) BWE_UpdateMousePosition: X="); display_print_dec((uint32_t)g_bwe_mouse_x);
            // display_print(" Y="); display_print_dec((uint32_t)g_bwe_mouse_y); display_print("\n");
            
            /* STEP 17: Instantly push updated coordinates to BSPE cursor plane */
            BSPE_SetCursorPosition(g_bwe_mouse_x, g_bwe_mouse_y);

            // 1. Let window manager process dragging/resizing state machine
            BWE_ProcessMouseInteraction(bwe_ev.data.mouse.x, bwe_ev.data.mouse.y, bwe_ev.data.mouse.buttons);

            // 2. Dispatch events to the topmost child window/control under the cursor
            extern bool BWE_IsDraggingActive(void);
            extern bool BWE_IsResizingActive(void);
            if (!BWE_IsDraggingActive() && !BWE_IsResizingActive()) {
                extern uint32_t g_z_order_stack[BWE_MAX_WINDOWS];
                extern uint32_t g_z_stack_count;
                static uint32_t s_hovered_control_id = 0;
                static uint32_t s_cached_z_version = 0;

                uint64_t ht_start = timer_get_ticks();
                /* STEP 14 */ uint64_t ht_start_tsc = step14_rdtsc(); /* END STEP 14 */

                // Recursive helper to find the leaf-most control under coordinates
                extern BWE_Window* BWE_GetWindow(uint32_t window_id);
                BWE_Window* target_win = 0;
                
                // --- O(1) Fast Path Cache (DISABLED) ---
                // This optimization is broken for hierarchical UI because a child control
                // is bounded within the parent window. If the parent window is cached,
                // moving the mouse over the child still counts as hitting the parent window,
                // thereby trapping the event and preventing it from reaching the child control!
#if 0
                if (s_hovered_control_id != 0 && s_hovered_control_id != BWE_DESKTOP_ID && g_z_order_version == s_cached_z_version) {
                    BWE_Window* hw = BWE_GetWindow(s_hovered_control_id);
                    if (hw && hw->state != BWE_STATE_HIDDEN && hw->state != BWE_STATE_DESTROYED) {
                        if (bwe_ev.data.mouse.x >= hw->screen_bounds.x &&
                            bwe_ev.data.mouse.x < hw->screen_bounds.x + hw->screen_bounds.width &&
                            bwe_ev.data.mouse.y >= hw->screen_bounds.y &&
                            bwe_ev.data.mouse.y < hw->screen_bounds.y + hw->screen_bounds.height) {
                            target_win = hw;
                        }
                    }
                }
#endif
                
                if (!target_win) {
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
                }

                uint32_t leaf_id = target_win ? target_win->id : BWE_DESKTOP_ID;
                s_cached_z_version = g_z_order_version;

                extern void display_print(const char*);
                display_print("[INPUT TRACE] HitTest\n");

                extern uint32_t g_hit_test_time_us;
                g_hit_test_time_us = (uint32_t)((timer_get_ticks() - ht_start) * 1000);
                /* STEP 14 */ step14_log_hit_test_done(step14_cycles_to_us(step14_rdtsc() - ht_start_tsc)); /* END STEP 14 */

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
                        extern uint32_t g_hud_hovered_control;
                        g_hud_hovered_control = leaf_id;
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
            // Dispatch keyboard events to the currently focused window/widget, or desktop
            extern uint32_t g_focused_window_id;
            uint32_t target_id = (g_focused_window_id == 0) ? BWE_DESKTOP_ID : g_focused_window_id;
            
            BWE_Window* target = BWE_GetWindow(target_id);
            if (target && target->on_event) {
                bwe_ev.target_id = target_id;
                target->on_event(target_id, &bwe_ev);
            }
        }
    }
    
    extern uint32_t g_pump_time_us;
    g_pump_time_us = (uint32_t)((timer_get_ticks() - pump_start) * 1000);
    /* STEP 14 */ step14_log_pump_done(step14_cycles_to_us(step14_rdtsc() - pump_start_tsc)); /* END STEP 14 */
}


// BWE_Compose Compatibility Wrapper
void BWE_Compose(void) {
    /* Phase D Stage 1: Read-only check against BDCE during composition trigger */
    BWE_AuditStage1_ReadOnly("BWE_Compose");
    extern BVFramebuffer* vbe_get_framebuffer(void);
    BWE_ComposeFrame(vbe_get_framebuffer());
}

// ============================================================
// Master Subsystem Initialize & Stage 1 Read-Only Audit
// ============================================================

#include "kernel/core/lib/include/string.h"

void BWE_AuditStage1_ReadOnly(const char* location) {
    const BDCE_Context* bdce_ctx = BDCE_GetPrimaryContext();
    if (!bdce_ctx) return;
    const BDCE_LogicalState* logical = BDCE_GetLogicalState(bdce_ctx);
    if (!logical) return;

    extern uint32_t g_kernel_screen_width;
    extern uint32_t g_kernel_screen_height;

    if (g_kernel_screen_width != logical->logical_width || g_kernel_screen_height != logical->logical_height) {
        static uint32_t s_audit_counter = 0;
        /* Rate-limit console output to first check and periodically (~once every 600 queries) */
        if ((s_audit_counter++ % 600) == 0) {
            display_print("[BWE_AUDIT_STAGE1] ");
            display_print(location ? location : "BWE");
            display_print(": Legacy global resolution (");
            display_print_dec(g_kernel_screen_width);
            display_print("x");
            display_print_dec(g_kernel_screen_height);
            display_print(") vs BDCE Authority (");
            display_print_dec(logical->logical_width);
            display_print("x");
            display_print_dec(logical->logical_height);
            display_print(") -> Discrepancy logged. Stage 1 Read-Only: NO MUTATION/REPAIR.\n");
        }
    }
}

bwe_error_t BWE_Initialize(void) {
    memset(g_windows, 0, sizeof(g_windows));
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

    extern uint32_t BOVISUAL_Graphics_GetWidth(void);
    extern uint32_t BOVISUAL_Graphics_GetHeight(void);
    desktop->local_bounds.x = 0;
    desktop->local_bounds.y = 0;
    desktop->local_bounds.width = (int32_t)BOVISUAL_Graphics_GetWidth();
    desktop->local_bounds.height = (int32_t)BOVISUAL_Graphics_GetHeight();
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

    /* Phase D Stage 1: Read-only constitutional comparison check against BDCE */
    BWE_AuditStage1_ReadOnly("BWE_Initialize");

    return BWE_SUCCESS;
}

// Compatibility Hooks for kernel.c graphical loop
void BOHeart_InputCapture(const BVEvent* event) {
    BOS_ProcessEvent(event);
    /* STEP 16: Immediately pump events to eliminate 16.6ms frame clock delay */
    extern void BWE_PumpEvents(void);
    BWE_PumpEvents();
}

typedef struct {
    int32_t mouse_x;
    int32_t mouse_y;
    uint8_t buttons;
    int32_t scroll;
} BWE_InputState;

extern BWE_InputState input_get_latest_state(void);

void BOHeart_Pulse(const BVFramebuffer* hw_fb) {
    extern void BWE_PumpEvents(void);
    BWE_PumpEvents();

    extern void animation_scheduler_update(uint32_t delta_time_ms);
    animation_scheduler_update(16);

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

