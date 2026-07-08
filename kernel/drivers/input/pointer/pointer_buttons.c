#include "pointer_buttons.h"
#include "kernel/drivers/display/display.h"

static uint32_t g_last_button_mask = 0;
static uint64_t g_last_release_ts = 0;
static int32_t  g_last_release_x = 0;
static int32_t  g_last_release_y = 0;
static uint8_t  g_current_click_count = 0;

static bool     g_is_dragging = false;
static int32_t  g_drag_start_x = 0;
static int32_t  g_drag_start_y = 0;
static uint32_t g_drag_button = 0;

void pointer_buttons_init(void) {
    g_last_button_mask = 0;
    g_last_release_ts = 0;
    g_last_release_x = 0;
    g_last_release_y = 0;
    g_current_click_count = 0;
    g_is_dragging = false;
    g_drag_start_x = 0;
    g_drag_start_y = 0;
    g_drag_button = 0;
    
    display_print("[POINTER BUTTONS] Drag & Transition State Machine Initialized.\n");
}

static uint32_t abs_diff(int32_t a, int32_t b) {
    return (a >= b) ? (uint32_t)(a - b) : (uint32_t)(b - a);
}

void pointer_buttons_process(uint32_t new_button_mask, int32_t current_x, int32_t current_y,
                             uint64_t timestamp_us, uint32_t* out_pressed, uint32_t* out_released,
                             uint8_t* out_click_count, bool* out_is_dragging,
                             int32_t* out_drag_start_x, int32_t* out_drag_start_y,
                             uint32_t* out_drag_button) {
    uint32_t pressed  = new_button_mask & ~g_last_button_mask;
    uint32_t released = ~new_button_mask & g_last_button_mask;

    // Evaluate Click Counts on primary button (bit 0) press
    if ((pressed & (1 << 0)) != 0) {
        if (g_last_release_ts != 0 && (timestamp_us - g_last_release_ts) <= POINTER_DOUBLE_CLICK_TIME_US) {
            if (abs_diff(current_x, g_last_release_x) <= POINTER_DRAG_THRESHOLD_PX &&
                abs_diff(current_y, g_last_release_y) <= POINTER_DRAG_THRESHOLD_PX) {
                g_current_click_count = (g_current_click_count == 1) ? 2 : ((g_current_click_count == 2) ? 3 : 1);
            } else {
                g_current_click_count = 1;
            }
        } else {
            g_current_click_count = 1;
        }
    }

    // Evaluate Drag Initiation (when any button is held)
    if (new_button_mask != 0) {
        if (g_last_button_mask == 0) {
            // First button pressed: record drag origin candidate
            g_drag_start_x = current_x;
            g_drag_start_y = current_y;
            g_drag_button = new_button_mask;
            g_is_dragging = false;
        } else if (!g_is_dragging) {
            // Check if movement exceeded drag threshold
            if (abs_diff(current_x, g_drag_start_x) >= POINTER_DRAG_THRESHOLD_PX ||
                abs_diff(current_y, g_drag_start_y) >= POINTER_DRAG_THRESHOLD_PX) {
                g_is_dragging = true;
            }
        }
    } else {
        // All buttons released: terminate drag
        if ((released & (1 << 0)) != 0) {
            g_last_release_ts = timestamp_us;
            g_last_release_x = current_x;
            g_last_release_y = current_y;
        }
        g_is_dragging = false;
        g_drag_button = 0;
    }

    g_last_button_mask = new_button_mask;

    if (out_pressed)      *out_pressed = pressed;
    if (out_released)     *out_released = released;
    if (out_click_count)  *out_click_count = (pressed != 0) ? g_current_click_count : 0;
    if (out_is_dragging)  *out_is_dragging = g_is_dragging;
    if (out_drag_start_x) *out_drag_start_x = g_drag_start_x;
    if (out_drag_start_y) *out_drag_start_y = g_drag_start_y;
    if (out_drag_button)  *out_drag_button = g_drag_button;
}
