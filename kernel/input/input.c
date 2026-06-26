#include "input.h"
#include "bmde.h"

// The global event queue
static BVEvent event_queue[MAX_EVENTS];
static volatile int queue_head = 0;
static volatile int queue_tail = 0;

// Global mouse state
static int32_t global_mouse_x = 0;
static int32_t global_mouse_y = 0;
static uint8_t global_mouse_buttons = 0;

// Hardcoded for now. In a real system, query the active display mode.
extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

void kernel_input_init(void) {
    queue_head = 0;
    queue_tail = 0;
    global_mouse_x = g_kernel_screen_width / 2;
    global_mouse_y = g_kernel_screen_height / 2;
    global_mouse_buttons = 0;
}

void kernel_input_update_resolution(uint32_t w, uint32_t h) {
    global_mouse_x = w / 2;
    global_mouse_y = h / 2;
}

static void push_event(const BVEvent* ev) {
    int next_head = (queue_head + 1) % MAX_EVENTS;
    if (next_head == queue_tail) {
        // Queue full, drop event
#ifdef BMDE_DEBUG
        bmde_state.dropped_events++;
#endif
        return;
    }
    event_queue[queue_head] = *ev;
    queue_head = next_head;
#ifdef BMDE_DEBUG
    bmde_state.queue_size = (queue_head >= queue_tail) ? (queue_head - queue_tail) : (MAX_EVENTS - queue_tail + queue_head);
#endif
}

bool kernel_get_event(BVEvent* out_event) {
    if (queue_head == queue_tail) {
        return false;
    }
    *out_event = event_queue[queue_tail];
    queue_tail = (queue_tail + 1) % MAX_EVENTS;
#ifdef BMDE_DEBUG
    bmde_state.queue_size = (queue_head >= queue_tail) ? (queue_head - queue_tail) : (MAX_EVENTS - queue_tail + queue_head);
#endif
    return true;
}

void kernel_input_push_mouse(int32_t dx, int32_t dy, uint8_t buttons) {
    global_mouse_x += dx;
    global_mouse_y -= dy; // PS/2 y-axis is bottom-up, screen is top-down

    if (global_mouse_x < 0) global_mouse_x = 0;
    if (global_mouse_y < 0) global_mouse_y = 0;
    if (global_mouse_x >= (int32_t)g_kernel_screen_width) global_mouse_x = (int32_t)g_kernel_screen_width - 1;
    if (global_mouse_y >= (int32_t)g_kernel_screen_height) global_mouse_y = (int32_t)g_kernel_screen_height - 1;

    // Check for movement
    BVEvent ev;
    ev.mouse_x = global_mouse_x;
    ev.mouse_y = global_mouse_y;
    ev.mouse_buttons = buttons;
    ev.key_code = 0;

    if (dx != 0 || dy != 0) {
        ev.type = BV_EVENT_MOUSE_MOVE;
        push_event(&ev);
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

void kernel_input_push_key(uint8_t scancode, bool is_pressed) {
    BVEvent ev;
    ev.type = is_pressed ? BV_EVENT_KEY_DOWN : BV_EVENT_KEY_UP;
    ev.mouse_x = global_mouse_x;
    ev.mouse_y = global_mouse_y;
    ev.mouse_buttons = global_mouse_buttons;
    ev.key_code = scancode;
    push_event(&ev);
}
