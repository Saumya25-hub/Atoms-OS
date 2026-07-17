#include "input.h"
#include "bmde.h"
#include "mouse_engine/mouse_engine.h"
#include "kernel/drivers/keyboard/include/keyboard.h"
#include "kernel/drivers/input/input_abstraction.h"
#include "drivers/input/usb_tablet/usb_tablet.h"
#include "drivers/input/vmmouse/vmmouse.h"
#include "kernel/debug/step14_telemetry.h"
#include "kernel/drivers/display/display.h"
#include "kernel/display/agdpe/agdpe.h"
#include "kernel/drivers/input/cursor/cursor_state.h"
#include "kernel/drivers/input/core/hida.h"

// The global event queue
static BVEvent event_queue[MAX_EVENTS];
static volatile int queue_head = 0;
static volatile int queue_tail = 0;
volatile uint64_t g_kernel_input_motion_coalesced = 0;
volatile uint32_t g_kernel_input_queue_peak = 0;

// Global mouse state
static int32_t global_mouse_x = 0;
static int32_t global_mouse_y = 0;
static uint8_t global_mouse_buttons = 0;

// Telemetry
uint32_t g_mouse_events_per_sec = 0;
uint32_t g_kbd_events_per_sec = 0;
uint32_t g_pump_time_us = 0;
uint32_t g_hit_test_time_us = 0;

// Hardcoded for now. In a real system, query the active display mode.
extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

static void push_event(const BVEvent* ev);

// Handler for KeyboardDriver callbacks
void kernel_input_push_key_event(KeyboardEvent* kevt) {
    if (!kevt) return;
    
    BVEvent ev;
    ev.type = kevt->pressed ? BV_EVENT_KEY_DOWN : BV_EVENT_KEY_UP;
    ev.mouse_x = global_mouse_x;
    ev.mouse_y = global_mouse_y;
    ev.mouse_buttons = global_mouse_buttons;
    
    // Preserve logical keycode independent of ascii character for BPDE
    ev.key_code = kevt->keycode;
    ev.ascii = kevt->ascii;
    ev.shift = kevt->shift;
    ev.ctrl = kevt->ctrl;
    ev.alt = kevt->alt;
    ev.caps_lock = kevt->caps_lock;
    
    extern uint32_t g_kbd_events_per_sec;
    g_kbd_events_per_sec++;
    
    push_event(&ev);
}

#include "arch/x86_64/io/port_io.h"

static uint32_t local_pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    io_out32(0xCF8, address);
    return io_in32(0xCFC);
}

static bool local_detect_usb_controller(void) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vd = local_pci_read_config((uint8_t)bus, slot, 0, 0);
            if (vd != 0xFFFFFFFF) {
                uint32_t class_code = local_pci_read_config((uint8_t)bus, slot, 0, 0x08);
                uint8_t base_class = (class_code >> 24) & 0xFF;
                uint8_t sub_class = (class_code >> 16) & 0xFF;
                if (base_class == 0x0C && sub_class == 0x03) {
                    return true;
                }
            }
        }
    }
    return false;
}

uint32_t kernel_input_get_queue_size(void) {
    if (queue_head >= queue_tail) {
        return queue_head - queue_tail;
    } else {
        return (MAX_EVENTS - queue_tail) + queue_head;
    }
}

#include "kernel/core/vizier/include/vizier.h"

void kernel_input_init(void) {
    VizierContract c = {0};
    c.subsystem_name = "INPUT_CORE";
    c.subsystem_id = VIZIER_SUBSYSTEM_INPUT_CORE;
    vizier_register_subsystem(&c);

    hida_init();
    extern void ccte_init(void);
    ccte_init();

    queue_head = 0;
    queue_tail = 0;
    global_mouse_x = g_kernel_screen_width / 2;
    global_mouse_y = g_kernel_screen_height / 2;
    global_mouse_buttons = 0;
    
    // VMMouse absolute mode initialization is deferred until after PS/2 mouse is reset and initialized in kernel.c.

    // Initialize V2 Engine
    mouse_engine_init(g_kernel_screen_width, g_kernel_screen_height);
    
    // Initialize BOMOUSETABUNDER abstraction layer
    input_abstraction_init(g_kernel_screen_width, g_kernel_screen_height);
    usb_tablet_init();
    
    // Hook keyboard driver
    keyboard_register_callback(kernel_input_push_key_event);
    
    // Phase 2 Input Core & Adapter initialization
    extern void input_adapter_init(void);
    input_adapter_init();
    
    // Phase 3 Pointer Engine initialization
    extern void pointer_engine_init(uint32_t screen_width, uint32_t screen_height);
    pointer_engine_init(g_kernel_screen_width, g_kernel_screen_height);
    
    // Phase 4 Event Dispatcher initialization
    extern void dispatcher_init(void);
    dispatcher_init();
    
    // Phase 5 Cursor Engine initialization
    extern void cursor_engine_init(uint32_t screen_width, uint32_t screen_height);
    cursor_engine_init(g_kernel_screen_width, g_kernel_screen_height);
    
    // Register Adapter as a consumer of Pointer Engine broadcasts & Event Dispatcher
    extern void input_adapter_register_pointer_consumer(void);
    input_adapter_register_pointer_consumer();
}

void kernel_input_update_resolution(uint32_t w, uint32_t h) {
    global_mouse_x = w / 2;
    global_mouse_y = h / 2;
    mouse_engine_update_resolution(w, h);
    input_abstraction_update_resolution(w, h);
    extern void cursor_engine_update_resolution(uint32_t screen_width, uint32_t screen_height);
    cursor_engine_update_resolution(w, h);
    extern void vmmouse_update_resolution(uint32_t screen_width, uint32_t screen_height);
    vmmouse_update_resolution(w, h);

    static bool s_printed_res_once = false;
    if (!s_printed_res_once) {
        s_printed_res_once = true;
        extern AGDPE_DisplayDevice* AGDPE_GetPrimaryDisplay(void);
        AGDPE_DisplayDevice* pdev = AGDPE_GetPrimaryDisplay();
        uint32_t phys_w = pdev ? pdev->width : w;
        uint32_t phys_h = pdev ? pdev->height : h;
        extern uint32_t g_kernel_screen_width;
        extern uint32_t g_kernel_screen_height;
        extern CursorState g_cursor_state;
        int32_t ptr_max_x = 0, ptr_max_y = 0;
        extern void pointer_bounds_get_max(int32_t* out_max_x, int32_t* out_max_y);
        pointer_bounds_get_max(&ptr_max_x, &ptr_max_y);
        uint32_t vm_w = 0, vm_h = 0;
        extern void vmmouse_get_bounds(uint32_t* out_w, uint32_t* out_h);
        vmmouse_get_bounds(&vm_w, &vm_h);

        display_print("\n=== RESOLUTION & BOUNDS VERIFICATION ===\n");
        display_print("Display:\nphysical_width="); display_print_dec(phys_w);
        display_print("\nphysical_height="); display_print_dec(phys_h);
        display_print("\n\nLogical:\ng_kernel_screen_width="); display_print_dec(g_kernel_screen_width);
        display_print("\ng_kernel_screen_height="); display_print_dec(g_kernel_screen_height);
        display_print("\n\nCursor:\ng_cursor_state.screen_width="); display_print_dec(g_cursor_state.screen_width);
        display_print("\ng_cursor_state.screen_height="); display_print_dec(g_cursor_state.screen_height);
        display_print("\n\nPointer Bounds:\npointer_max_x="); display_print_dec(ptr_max_x);
        display_print("\npointer_max_y="); display_print_dec(ptr_max_y);
        display_print("\n\nVMMouse:\nabsolute_max_x="); display_print_dec((vm_w > 0) ? (vm_w - 1) : 0);
        display_print("\nabsolute_max_y="); display_print_dec((vm_h > 0) ? (vm_h - 1) : 0);
        display_print("\n========================================\n\n");
    }
}

volatile uint64_t g_input_events_count = 0;

static void push_event(const BVEvent* ev) {
    /* Motion is state, not a lossless stream.  Keep only the newest pending
       position, but never merge across a button transition. */
    if (ev->type == BV_EVENT_MOUSE_MOVE && queue_head != queue_tail) {
        int last = (queue_head == 0) ? (MAX_EVENTS - 1) : (queue_head - 1);
        if (event_queue[last].type == BV_EVENT_MOUSE_MOVE) {
            event_queue[last] = *ev;
            g_kernel_input_motion_coalesced++;
            return;
        }
    }

    int next_head = (queue_head + 1) % MAX_EVENTS;
    if (next_head == queue_tail) {
        // Queue full, drop event
#ifdef BMDE_DEBUG
        bmde_state.dropped_events++;
#endif
        return;
    }
    g_input_events_count++;
    event_queue[queue_head] = *ev;
    queue_head = next_head;
    /* STEP 16 TELEMETRY */
    step14_log_irq();
    int qlen = (queue_head >= queue_tail) ? (queue_head - queue_tail) : (MAX_EVENTS - queue_tail + queue_head);
    if ((uint32_t)qlen > g_kernel_input_queue_peak) {
        g_kernel_input_queue_peak = (uint32_t)qlen;
    }
    step14_log_queue_push(qlen);
    /* END STEP 16 */
#ifdef BMDE_DEBUG
    bmde_state.queue_size = (queue_head >= queue_tail) ? (queue_head - queue_tail) : (MAX_EVENTS - queue_tail + queue_head);
#endif
}

bool kernel_get_event(BVEvent* out_event) {
    __asm__ volatile("cli");
    if (queue_head == queue_tail) {
        __asm__ volatile("sti");
        return false;
    }
    *out_event = event_queue[queue_tail];
    queue_tail = (queue_tail + 1) % MAX_EVENTS;
#ifdef BMDE_DEBUG
    bmde_state.queue_size = (queue_head >= queue_tail) ? (queue_head - queue_tail) : (MAX_EVENTS - queue_tail + queue_head);
#endif
    __asm__ volatile("sti");
    return true;
}

void kernel_input_push_mouse_absolute(int32_t abs_x, int32_t abs_y, uint8_t buttons) {
    // Check for movement by diffing absolute positions
    int32_t dx = abs_x - global_mouse_x;
    // dy logic (subtraction) isn't needed here because abs_y is already clamped and computed by pointer_manager.
    // We just check if they are different.
    
    bool moved = (abs_x != global_mouse_x) || (abs_y != global_mouse_y);

    global_mouse_x = abs_x;
    global_mouse_y = abs_y;

    BVEvent ev;
    ev.mouse_x = global_mouse_x;
    ev.mouse_y = global_mouse_y;
    ev.mouse_buttons = buttons;
    ev.key_code = 0;

    if (moved) {
        ev.type = BV_EVENT_MOUSE_MOVE;
        push_event(&ev);
        extern uint32_t g_mouse_events_per_sec;
        g_mouse_events_per_sec++;
    }

    // Check for button state changes
    for (int i = 0; i < 3; i++) {
        bool was_pressed = (global_mouse_buttons & (1 << i)) != 0;
        bool is_pressed = (buttons & (1 << i)) != 0;

        if (is_pressed && !was_pressed) {
            ev.type = BV_EVENT_MOUSE_DOWN;
            push_event(&ev);
        } else if (!is_pressed && was_pressed) {
            ev.type = BV_EVENT_MOUSE_UP;
            push_event(&ev);
        }
    }

    global_mouse_buttons = buttons;
}

void kernel_input_push_mouse(int32_t dx, int32_t dy, uint8_t buttons) {
    // Compatibility Layer: Route to V2
    mouse_engine_push_packet(dx, dy, buttons, false, false);
}

void kernel_input_push_key(uint8_t scancode, bool is_pressed) {
    BVEvent ev;
    ev.type = is_pressed ? BV_EVENT_KEY_DOWN : BV_EVENT_KEY_UP;
    ev.mouse_x = global_mouse_x;
    ev.mouse_y = global_mouse_y;
    ev.mouse_buttons = global_mouse_buttons;
    ev.key_code = scancode;
    push_event(&ev);
}
