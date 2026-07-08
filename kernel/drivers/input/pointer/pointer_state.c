#include "pointer_state.h"
#include "kernel/drivers/display/display.h"

static PointerState g_pointer_state;

void pointer_state_init(uint32_t initial_x, uint32_t initial_y) {
    g_pointer_state.current_x = (int32_t)initial_x;
    g_pointer_state.current_y = (int32_t)initial_y;
    g_pointer_state.previous_x = (int32_t)initial_x;
    g_pointer_state.previous_y = (int32_t)initial_y;
    
    g_pointer_state.raw_x = (int32_t)initial_x;
    g_pointer_state.raw_y = (int32_t)initial_y;
    g_pointer_state.subpixel_x = ((int32_t)initial_x) << 16;
    g_pointer_state.subpixel_y = ((int32_t)initial_y) << 16;
    
    g_pointer_state.delta_x = 0;
    g_pointer_state.delta_y = 0;
    g_pointer_state.velocity_raw = 0;
    g_pointer_state.acceleration_factor = 1 << 16; // 1.0x in 16.16 fixed-point
    
    g_pointer_state.button_mask = 0;
    g_pointer_state.button_just_pressed = 0;
    g_pointer_state.button_just_released = 0;
    g_pointer_state.click_count = 0;
    
    g_pointer_state.is_dragging = false;
    g_pointer_state.drag_start_x = 0;
    g_pointer_state.drag_start_y = 0;
    g_pointer_state.drag_button = 0;
    
    g_pointer_state.capture_window_id = 0;
    g_pointer_state.hover_window_id = 0;
    g_pointer_state.timestamp_us = 0;
    g_pointer_state.device_source_id = 0;
    g_pointer_state.pointer_id = 1; // Default primary pointer
    
    display_print("[POINTER STATE] Authoritative PointerState Singleton Initialized.\n");
}

const PointerState* pointer_state_get(void) {
    return &g_pointer_state;
}

PointerState* pointer_state_get_mutable(void) {
    return &g_pointer_state;
}

void pointer_state_update_position(int32_t x, int32_t y, int32_t raw_x, int32_t raw_y,
                                   int32_t sub_x, int32_t sub_y, int32_t dx, int32_t dy,
                                   int32_t vel, int32_t accel, uint64_t ts, uint32_t dev_id) {
    g_pointer_state.previous_x = g_pointer_state.current_x;
    g_pointer_state.previous_y = g_pointer_state.current_y;
    g_pointer_state.current_x = x;
    g_pointer_state.current_y = y;
    
    g_pointer_state.raw_x = raw_x;
    g_pointer_state.raw_y = raw_y;
    g_pointer_state.subpixel_x = sub_x;
    g_pointer_state.subpixel_y = sub_y;
    
    g_pointer_state.delta_x = dx;
    g_pointer_state.delta_y = dy;
    g_pointer_state.velocity_raw = vel;
    g_pointer_state.acceleration_factor = accel;
    
    g_pointer_state.timestamp_us = ts;
    g_pointer_state.device_source_id = dev_id;
}

void pointer_state_update_buttons(uint32_t mask, uint32_t pressed, uint32_t released,
                                  uint8_t clicks, bool dragging, int32_t drag_x,
                                  int32_t drag_y, uint32_t drag_btn) {
    g_pointer_state.button_mask = mask;
    g_pointer_state.button_just_pressed = pressed;
    g_pointer_state.button_just_released = released;
    g_pointer_state.click_count = clicks;
    
    g_pointer_state.is_dragging = dragging;
    g_pointer_state.drag_start_x = drag_x;
    g_pointer_state.drag_start_y = drag_y;
    g_pointer_state.drag_button = drag_btn;
}
