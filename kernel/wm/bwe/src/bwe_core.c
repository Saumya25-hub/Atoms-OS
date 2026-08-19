#include "../include/bwe.h"
#include "kernel/debug/step14_telemetry.h"
#include "kernel/graphics/BSPE/include/bspe.h"
#include "kernel/display/agdae/agdae.h"
#include "kernel/display/bdce/include/bdce_authority.h"
#include "kernel/display/bdce/include/bdce_context.h"
#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/lib/include/string.h"
#include "../include/bwe_process_queue.h"

// External kernel display printing APIs
extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);

// BWE Static Master Pool
BWE_Window g_windows[BWE_MAX_WINDOWS];
volatile uint64_t g_bwe_motion_events_coalesced = 0;
volatile uint32_t g_bwe_event_queue_peak = 0;
static uint16_t   g_window_generations[BWE_MAX_WINDOWS];
static uint32_t   g_active_window_count = 0;

// Event Queue State
static BWE_EventQueue g_event_queue;

// Global Focus State
uint32_t g_active_window_id = BWE_DESKTOP_ID;
uint32_t g_focused_window_id = BWE_DESKTOP_ID;

static uint32_t s_hovered_control_id = 0;
static uint32_t s_cached_z_version = 0;

void BWE_ResetHoverCache(uint32_t window_id) {
    if (window_id == 0 || s_hovered_control_id == window_id) {
        s_hovered_control_id = 0;
        s_cached_z_version = 0;
    } else {
        // If window_id is destroyed or invalid, clear cache unconditionally to prevent leaks
        BWE_Window* hw = BWE_GetWindow(s_hovered_control_id);
        if (!hw || hw->state == BWE_STATE_DESTROYED || hw->state == BWE_STATE_HIDDEN) {
            s_hovered_control_id = 0;
            s_cached_z_version = 0;
        }
    }
}

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
void bwe_log(const char* level, const char* msg) {
    extern bool audio_player_is_playing(void);
    if (audio_player_is_playing() && level && level[0] != 'E' && level[0] != 'W' && level[0] != 'A' && level[0] != 'F') return;
    
    display_print("[BWE_");
    display_print(level);
    display_print("] ");
    display_print(msg);
    display_print("\n");
}

void bwe_log_id(const char* level, const char* msg, uint32_t id) {
    extern bool audio_player_is_playing(void);
    if (audio_player_is_playing() && level && level[0] != 'E' && level[0] != 'W' && level[0] != 'A' && level[0] != 'F') return;

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
    extern void BSCE_MarkSlotDirty(uint32_t surface_id);
    BSCE_MarkSlotDirty(win->id);
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

#ifndef BWE_ENABLE_RENDER_TRACE
#define BWE_ENABLE_RENDER_TRACE 0
#endif

#if BWE_ENABLE_RENDER_TRACE
    extern void serial_write_direct(const char* str);
    extern void serial_write_dec_direct(int val);
    serial_write_direct("[RENDER_TRACE 2] BWE_InvalidateWindow setting is_dirty=true for ID=");
    serial_write_dec_direct((int)window_id);
    serial_write_direct("\n");
#endif

    // Recursively invalidate all descendants
    invalidate_descendants_recursive(win);
    
    // Add damaged region to compositor dirty list
    extern void BWE_AddCompositorDirtyRect(const BWE_Rect* rect);
    BWE_AddCompositorDirtyRect(&win->screen_bounds);

    // Bubble up to parents so they are also recomposed
    uint32_t curr_parent = win->parent_id;
    while (curr_parent != BWE_DESKTOP_ID && curr_parent != win->id) {
        BWE_Window* p = BWE_GetWindow(curr_parent);
        if (p) {
            p->is_dirty = true;
            extern void BSCE_MarkSlotDirty(uint32_t surface_id);
            BSCE_MarkSlotDirty(p->id);
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
            BWE_Rect parent_client;
            extern void BWE_Geometry_CalculateClientBounds(BWE_Window* w, BWE_Rect* o);
            BWE_Geometry_CalculateClientBounds(parent, &parent_client);
            win->screen_bounds.x = parent_client.x + win->local_bounds.x;
            win->screen_bounds.y = parent_client.y + win->local_bounds.y;
            
            // Recapture baseline margins
            win->margins.left = win->local_bounds.x;
            win->margins.top = win->local_bounds.y;
            win->margins.right = parent_client.width - (win->local_bounds.x + win->local_bounds.width);
            win->margins.bottom = parent_client.height - (win->local_bounds.y + win->local_bounds.height);
            win->baseline_parent_w = parent_client.width;
            win->baseline_parent_h = parent_client.height;
            
            extern void BWE_Diag_GuardBounds(BWE_Window* p, BWE_Window* c);
            BWE_Diag_GuardBounds(parent, win);
        } else {
            win->screen_bounds.x = win->local_bounds.x;
            win->screen_bounds.y = win->local_bounds.y;
            win->baseline_parent_w = 0;
            win->baseline_parent_h = 0;
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

    /* Keep mouse motion bounded all the way to the window manager.  Button
       edges and keyboard events retain strict FIFO ordering. */
    if (event->type == BWE_EVENT_MOUSE_MOVE && g_event_queue.count > 0) {
        uint32_t last = (g_event_queue.tail == 0) ? (BWE_EVENT_QUEUE_SIZE - 1) : (g_event_queue.tail - 1);
        if (g_event_queue.events[last].type == BWE_EVENT_MOUSE_MOVE) {
            g_event_queue.events[last] = *event;
            g_bwe_motion_events_coalesced++;
            return BWE_SUCCESS;
        }
    }

    if (g_event_queue.count >= BWE_EVENT_QUEUE_SIZE) {
        return BWE0006;
    }

    g_event_queue.events[g_event_queue.tail] = *event;
    g_event_queue.tail = (g_event_queue.tail + 1) % BWE_EVENT_QUEUE_SIZE;
    g_event_queue.count++;
    if (g_event_queue.count > g_bwe_event_queue_peak) {
        g_bwe_event_queue_peak = g_event_queue.count;
    }

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

static void dispatch_to_process_queue(BWE_Window* win, const BWE_Event* bwe_ev) {
    if (!win || !bwe_ev) return;
    
    BOS_InputEvent out_ev;
    out_ev.window_id = win->id;
    extern uint64_t timer_get_ticks(void);
    out_ev.timestamp = timer_get_ticks();
    
    if (bwe_ev->type == BWE_EVENT_KEY_DOWN || bwe_ev->type == BWE_EVENT_KEY_UP) {
        out_ev.type = (bwe_ev->type == BWE_EVENT_KEY_DOWN) ? BOS_INPUT_KEY_DOWN : BOS_INPUT_KEY_UP;
        out_ev.data.key.key = bwe_ev->data.key.key_code;
        out_ev.data.key.character = bwe_ev->data.key.character;
        out_ev.data.key.scancode = bwe_ev->data.key.scancode;
        out_ev.data.key.modifiers = bwe_ev->data.key.modifiers;
        out_ev.data.key.repeat = 0;
    } else if (bwe_ev->type == BWE_EVENT_MOUSE_MOVE) {
        out_ev.type = BOS_INPUT_MOUSE_MOVE;
        out_ev.data.mouse.screen_x = bwe_ev->data.mouse.x;
        out_ev.data.mouse.screen_y = bwe_ev->data.mouse.y;
        out_ev.data.mouse.local_x = bwe_ev->data.mouse.x - win->screen_bounds.x;
        out_ev.data.mouse.local_y = bwe_ev->data.mouse.y - win->screen_bounds.y;
        out_ev.data.mouse.delta_x = 0; 
        out_ev.data.mouse.delta_y = 0;
        out_ev.data.mouse.buttons = bwe_ev->data.mouse.buttons;
    } else if (bwe_ev->type == BWE_EVENT_MOUSE_DOWN || bwe_ev->type == BWE_EVENT_MOUSE_UP) {
        out_ev.type = (bwe_ev->type == BWE_EVENT_MOUSE_DOWN) ? BOS_INPUT_MOUSE_DOWN : BOS_INPUT_MOUSE_UP;
        out_ev.data.mouse.screen_x = bwe_ev->data.mouse.x;
        out_ev.data.mouse.screen_y = bwe_ev->data.mouse.y;
        out_ev.data.mouse.local_x = bwe_ev->data.mouse.x - win->screen_bounds.x;
        out_ev.data.mouse.local_y = bwe_ev->data.mouse.y - win->screen_bounds.y;
        out_ev.data.mouse.delta_x = 0;
        out_ev.data.mouse.delta_y = 0;
        out_ev.data.mouse.buttons = bwe_ev->data.mouse.buttons;
    } else {
        return;
    }
    
    bwe_process_queue_push(win->owner_pid, &out_ev);
}

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
            bwe_ev.data.key.character = event->ascii;
            bwe_ev.data.key.scancode = 0; // BVEvent lacks this currently, will fix in InputCoreEvent
            bwe_ev.data.key.modifiers = (event->shift ? 1 : 0) | (event->ctrl ? 2 : 0) | (event->alt ? 4 : 0);
            break;
        case BV_EVENT_KEY_UP:
            bwe_ev.type = BWE_EVENT_KEY_UP;
            bwe_ev.data.key.key_code = event->key_code;
            bwe_ev.data.key.character = event->ascii;
            bwe_ev.data.key.scancode = 0;
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

void BWE_PumpEvents(void);

static bool bre_input_pump_callback(uint32_t budget) {
    (void)budget;
    extern void xhci_poll(void);
    extern void vmmouse_poll(void);
    extern void input_adapter_pump(void);
    extern void input_core_dispatch_events(void);
    extern void dispatcher_pump_events(void);

    xhci_poll();
    vmmouse_poll();
    input_adapter_pump();
    input_core_dispatch_events();
    dispatcher_pump_events();
    BWE_PumpEvents();

    return false;
}

void BWE_PumpEvents(void) {
    static bool s_bre_input_registered = false;
    if (!s_bre_input_registered) {
        s_bre_input_registered = true;
        extern void BRE_RegisterService(uint32_t id, bool (*callback)(uint32_t), uint32_t default_budget);
        BRE_RegisterService(1, bre_input_pump_callback, 16);
    }

    extern uint64_t timer_get_ticks(void);
    uint64_t pump_start = timer_get_ticks();
    /* STEP 14 */ uint64_t pump_start_tsc = step14_rdtsc(); /* END STEP 14 */
    /* STEP 16 */ step14_log_pump_start(g_step14_telemetry.last_irq_timestamp_ms); /* END STEP 16 */
    
    BWE_Event bwe_ev;
    uint32_t processed = 0;
    const uint32_t budget = 64;
    while (processed < budget && BWE_EventQueue_Pop(&bwe_ev) == BWE_SUCCESS) {
        processed++;
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
            extern void BWE_ProcessMouseInteraction(int32_t mouse_x, int32_t mouse_y, uint8_t buttons, uint32_t event_type);
            BWE_ProcessMouseInteraction(bwe_ev.data.mouse.x, bwe_ev.data.mouse.y, bwe_ev.data.mouse.buttons, bwe_ev.type);

            // 2. Dispatch events to the topmost child window/control under the cursor
            extern bool BWE_IsDraggingActive(void);
            extern bool BWE_IsResizingActive(void);
            if (!BWE_IsDraggingActive() && !BWE_IsResizingActive()) {
                extern uint32_t g_z_order_stack[BWE_MAX_WINDOWS];
                extern uint32_t g_z_stack_count;

                uint64_t ht_start = timer_get_ticks();
                /* STEP 14 */ uint64_t ht_start_tsc = step14_rdtsc(); /* END STEP 14 */

                // Recursive helper to find the leaf-most control under coordinates
                extern BWE_Window* BWE_GetWindow(uint32_t window_id);
                BWE_Window* target_win = 0;
                
                // --- O(1) Leaf Control Fast Path Cache ---
                if (s_hovered_control_id != 0 && s_hovered_control_id != BWE_DESKTOP_ID && g_z_order_version == s_cached_z_version) {
                    BWE_Window* hw = BWE_GetWindow(s_hovered_control_id);
                    if (hw && hw->state != BWE_STATE_HIDDEN && hw->state != BWE_STATE_DESTROYED && hw->child_count == 0) {
                        if (bwe_ev.data.mouse.x >= hw->screen_bounds.x &&
                            bwe_ev.data.mouse.x < hw->screen_bounds.x + hw->screen_bounds.width &&
                            bwe_ev.data.mouse.y >= hw->screen_bounds.y &&
                            bwe_ev.data.mouse.y < hw->screen_bounds.y + hw->screen_bounds.height) {
                            target_win = hw;
                        }
                    } else {
                        // Invalidate stale hover cache immediately
                        s_hovered_control_id = 0;
                        s_cached_z_version = 0;
                    }
                }
                
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
                                        if (child && child->state != BWE_STATE_HIDDEN && child->state != BWE_STATE_DESTROYED) {
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
                extern volatile uint32_t g_desktop_shell_win_id;
                if (leaf_id == BWE_DESKTOP_ID && g_desktop_shell_win_id != 0 && BWE_ValidateWindow(g_desktop_shell_win_id)) {
                    leaf_id = g_desktop_shell_win_id;
                }

                extern uint32_t g_hit_test_time_us;
                g_hit_test_time_us = (uint32_t)((timer_get_ticks() - ht_start) * 1000);
                /* STEP 14 */ step14_log_hit_test_done(step14_cycles_to_us(step14_rdtsc() - ht_start_tsc)); /* END STEP 14 */

                // Hover state tracking (MOUSE_ENTER / MOUSE_LEAVE)
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

                // Focus routing on click: resolve top-level parent window so control clicks do not deactivate the parent window
                if (bwe_ev.type == BWE_EVENT_MOUSE_DOWN && target_win) {
                    extern bwe_error_t BOS_SetFocus(uint32_t window_id);
                    uint32_t top_id = target_win->id;
                    BWE_Window* curr_top = target_win;
                    while (curr_top && curr_top->parent_id != BWE_DESKTOP_ID && curr_top->parent_id != curr_top->id && curr_top->parent_id != 0) {
                        top_id = curr_top->parent_id;
                        curr_top = BWE_GetWindow(top_id);
                    }
                    if (top_id != BWE_DESKTOP_ID) {
                        BOS_SetFocus(top_id);
                    }
                }

                // Synchronize cached Z-order version AFTER focus changes/Z-reordering
                s_cached_z_version = g_z_order_version;

                // Dispatch mouse event to the target leaf-most window/control
                BWE_Window* dispatch_target = BWE_GetWindow(leaf_id);
                if (dispatch_target) {
                    static bool s_hittest_logged = false;
                    if (!s_hittest_logged && leaf_id != BWE_DESKTOP_ID) {
                        s_hittest_logged = true;
                        extern void com1_puts(const char* s);
                        com1_puts("[BWE_GUI] HITTEST PASS\r\n");
                    }
                    dispatch_to_process_queue(dispatch_target, &bwe_ev);
                    if (dispatch_target->on_event) {
                        bwe_ev.target_id = leaf_id;
                        dispatch_target->on_event(leaf_id, &bwe_ev);
                    }

                    // Forward to Ring 3 Syscall Event Queue
                    extern void sys_gui_post_event(uint32_t win_id, const BOS_GUIEvent *ev);
                    BOS_GUIEvent gui_ev;
                    memset(&gui_ev, 0, sizeof(gui_ev));
                    gui_ev.abi_version = BOS_GUI_EVENT_ABI_VERSION;
                    gui_ev.window_id = leaf_id;
                    gui_ev.mouse_x = bwe_ev.data.mouse.x - dispatch_target->screen_bounds.x;
                    gui_ev.mouse_y = bwe_ev.data.mouse.y - dispatch_target->screen_bounds.y;
                    gui_ev.mouse_btn = bwe_ev.data.mouse.buttons;
                    if (bwe_ev.type == BWE_EVENT_MOUSE_DOWN) gui_ev.type = BOS_GUI_EVENT_MOUSE_DOWN;
                    else if (bwe_ev.type == BWE_EVENT_MOUSE_UP) gui_ev.type = BOS_GUI_EVENT_MOUSE_UP;
                    else if (bwe_ev.type == BWE_EVENT_MOUSE_MOVE) gui_ev.type = BOS_GUI_EVENT_MOUSE_MOVE;
                    sys_gui_post_event(leaf_id, &gui_ev);

                    static bool s_event_route_logged = false;
                    if (!s_event_route_logged && leaf_id != BWE_DESKTOP_ID) {
                        s_event_route_logged = true;
                        extern void com1_puts(const char* s);
                        com1_puts("[BWE_GUI] EVENT ROUTE PASS\r\n");
                    }
                }

#ifndef BWE_ENABLE_CLICK_TRACE
#define BWE_ENABLE_CLICK_TRACE 0
#endif

#if BWE_ENABLE_CLICK_TRACE
                if (bwe_ev.type == BWE_EVENT_MOUSE_DOWN || bwe_ev.type == BWE_EVENT_MOUSE_UP) {
                    extern void serial_write_direct(const char* str);
                    extern void serial_write_dec_direct(int val);
                    
                    serial_write_direct("\n=== CLICK TRACE ===\n");
                    serial_write_direct("Event Type       : ");
                    serial_write_direct(bwe_ev.type == BWE_EVENT_MOUSE_DOWN ? "MOUSE_DOWN\n" : "MOUSE_UP\n");
                    serial_write_direct("Cursor Position  : (X=");
                    serial_write_dec_direct(bwe_ev.data.mouse.x);
                    serial_write_direct(", Y=");
                    serial_write_dec_direct(bwe_ev.data.mouse.y);
                    serial_write_direct(")\n");
                    
                    uint32_t top_win_id = target_win ? target_win->id : 0;
                    BWE_Window* tw = target_win;
                    while (tw && tw->parent_id != BWE_DESKTOP_ID && tw->parent_id != tw->id && tw->parent_id != 0) {
                        top_win_id = tw->parent_id;
                        tw = BWE_GetWindow(top_win_id);
                    }
                    serial_write_direct("Top Window       : ");
                    serial_write_dec_direct((int)top_win_id);
                    serial_write_direct("\n");
                    
                    extern uint32_t g_focused_window_id;
                    serial_write_direct("Focused Window   : ");
                    serial_write_dec_direct((int)g_focused_window_id);
                    serial_write_direct("\n");
                    
                    serial_write_direct("Captured Window  : NONE\n");
                    
                    serial_write_direct("Hit-Test Window  : ");
                    serial_write_dec_direct((int)top_win_id);
                    serial_write_direct("\n");
                    
                    serial_write_direct("Hit-Test Control : ");
                    serial_write_dec_direct((int)leaf_id);
                    serial_write_direct("\n");

                    serial_write_direct("Control Type     : ");
                    if (dispatch_target) {
                        switch (dispatch_target->type) {
                            case BWE_TYPE_BUTTON:   serial_write_direct("BUTTON\n");   break;
                            case BWE_TYPE_PANEL:    serial_write_direct("PANEL\n");    break;
                            case BWE_TYPE_CHECKBOX: serial_write_direct("CHECKBOX\n"); break;
                            case BWE_TYPE_LABEL:    serial_write_direct("LABEL\n");    break;
                            case BWE_TYPE_WINDOW:   serial_write_direct("WINDOW\n");   break;
                            case BWE_TYPE_CANVAS:   serial_write_direct("CANVAS\n");   break;
                            default:                serial_write_direct("OTHER\n");    break;
                        }
                    } else {
                        serial_write_direct("NULL\n");
                    }

                    serial_write_direct("Generated Message: ");
                    serial_write_dec_direct((int)bwe_ev.type);
                    serial_write_direct("\n");
                    
                    bool delivered = (dispatch_target != 0 && dispatch_target->on_event != 0);
                    serial_write_direct("Delivered?       : ");
                    serial_write_direct(delivered ? "YES\n" : "NO\n");
                    
                    bool is_btn = (dispatch_target && dispatch_target->type == BWE_TYPE_BUTTON);
                    bool btn_pressed = is_btn ? dispatch_target->control_data.button.is_pressed : false;
                    serial_write_direct("Control IsPressed: ");
                    serial_write_direct(btn_pressed ? "TRUE\n" : "FALSE\n");
                    serial_write_direct("===================\n");
                }
#endif
            }
        } else {
            // Dispatch keyboard events to the currently focused window/widget, or desktop
            extern uint32_t g_focused_window_id;
            uint32_t target_id = (g_focused_window_id == 0) ? BWE_DESKTOP_ID : g_focused_window_id;
            
            BWE_Window* target = BWE_GetWindow(target_id);
            if (target) {
                dispatch_to_process_queue(target, &bwe_ev);
                if (target->on_event) {
                    bwe_ev.target_id = target_id;
                    target->on_event(target_id, &bwe_ev);
                }

                // Forward to Ring 3 Syscall Event Queue
                extern void sys_gui_post_event(uint32_t win_id, const BOS_GUIEvent *ev);
                BOS_GUIEvent gui_ev;
                memset(&gui_ev, 0, sizeof(gui_ev));
                gui_ev.abi_version = BOS_GUI_EVENT_ABI_VERSION;
                gui_ev.window_id = target_id;
                gui_ev.key_code = bwe_ev.data.key.key_code;
                gui_ev.ascii_char = bwe_ev.data.key.character;
                gui_ev.modifiers = bwe_ev.data.key.modifiers;
                if (bwe_ev.type == BWE_EVENT_KEY_DOWN) gui_ev.type = BOS_GUI_EVENT_KEY_DOWN;
                else if (bwe_ev.type == BWE_EVENT_KEY_UP) gui_ev.type = BOS_GUI_EVENT_KEY_UP;
                sys_gui_post_event(target_id, &gui_ev);
            }
        }
    }
    
    if (processed > 0) {
        extern void BWE_Compose(void);
        BWE_Compose();
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

#include "kernel/core/vizier/include/vizier.h"

bwe_error_t BWE_Initialize(void) {
    VizierContract c = {0};
    c.subsystem_name = "BWE";
    c.subsystem_id = VIZIER_SUBSYSTEM_BWE;
    vizier_register_subsystem(&c);

    bwe_log("INFO", "Initializing BWE Subsystem v2.0...");
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

    bwe_process_queue_init();

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

    /* Per-frame app ticks — drives canvas-based media rendering */
    extern void bos_media_player_tick(void);
    bos_media_player_tick();

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

/* BOS_GetApplication implemented in kernel/application/app_manager/app_manager.c */

void* BWE_GetSurface(uint32_t id) {
    (void)id;
    return 0;
}

void BOS_SetText(uint32_t id, const char* text) {
    if (!text) return;
    BWE_Window* win = BWE_GetWindow(id);
    if (!win) return;

    size_t len = 0;
    while (text[len] != '\0' && len < sizeof(win->title) - 1) {
        win->title[len] = text[len];
        len++;
    }
    win->title[len] = '\0';
    win->is_dirty = true;
}

#if 0
bool bos_gui_event_pop(uint32_t id, void* ev) {
    (void)id;
    (void)ev;
    return false;
}
#endif

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

