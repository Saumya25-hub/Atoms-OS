#include "input_abstraction.h"
#include "input.h"
#include "kernel/drivers/display/display.h"

static InputState g_latest_state = {0, 0, 0, 0};
static uint32_t g_screen_width = 1280;
static uint32_t g_screen_height = 720;

extern void kernel_input_push_mouse_absolute(int32_t abs_x, int32_t abs_y, uint8_t buttons);

void input_abstraction_init(uint32_t screen_width, uint32_t screen_height) {
    g_screen_width = (screen_width > 0) ? screen_width : 1280;
    g_screen_height = (screen_height > 0) ? screen_height : 720;
    
    g_latest_state.mouse_x = g_screen_width / 2;
    g_latest_state.mouse_y = g_screen_height / 2;
    g_latest_state.buttons = 0;
    g_latest_state.scroll = 0;
    
    display_print("[BOMOUSETABUNDER] Input Abstraction Layer Initialized.\n");
}

void input_abstraction_update_resolution(uint32_t screen_width, uint32_t screen_height) {
    g_screen_width = (screen_width > 0) ? screen_width : 1280;
    g_screen_height = (screen_height > 0) ? screen_height : 720;
    
    if (g_latest_state.mouse_x >= (int32_t)g_screen_width) {
        g_latest_state.mouse_x = (int32_t)g_screen_width - 1;
    }
    if (g_latest_state.mouse_y >= (int32_t)g_screen_height) {
        g_latest_state.mouse_y = (int32_t)g_screen_height - 1;
    }
}

void input_push_absolute(int32_t abs_x, int32_t abs_y, uint8_t buttons, int32_t scroll_delta) {
    extern void serial_write_direct(const char* str);
    extern void serial_write_dec_direct(int val);
    serial_write_direct("[ABS TRACE] input_push_absolute: x=");
    serial_write_dec_direct(abs_x);
    serial_write_direct(" y=");
    serial_write_dec_direct(abs_y);
    serial_write_direct(" btns=");
    serial_write_dec_direct(buttons);
    serial_write_direct("\n");

    if (abs_x < 0) abs_x = 0;
    if (abs_x >= (int32_t)g_screen_width) abs_x = (int32_t)g_screen_width - 1;
    if (abs_y < 0) abs_y = 0;
    if (abs_y >= (int32_t)g_screen_height) abs_y = (int32_t)g_screen_height - 1;
    
    g_latest_state.mouse_x = abs_x;
    g_latest_state.mouse_y = abs_y;
    g_latest_state.buttons = buttons;
    g_latest_state.scroll += scroll_delta;
    
    // Bridge to legacy kernel event queue for discrete event handling
    kernel_input_push_mouse_absolute(abs_x, abs_y, buttons);
}

void input_push_relative(int32_t dx, int32_t dy, uint8_t buttons, int32_t scroll_delta) {
    int32_t new_x = g_latest_state.mouse_x + dx;
    int32_t new_y = g_latest_state.mouse_y - dy; // PS/2 Y coordinate is bottom-up
    
    input_push_absolute(new_x, new_y, buttons, scroll_delta);
}

InputState input_get_latest_state(void) {
    return g_latest_state;
}
