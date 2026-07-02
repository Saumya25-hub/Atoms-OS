#include "usb_tablet.h"
#include "kernel/drivers/input/input_abstraction.h"
#include "kernel/drivers/display/display.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

void usb_tablet_init(void) {
    display_print("[USB-TABLET] Primary Absolute Input Driver Initialized.\n");
}

void usb_tablet_report_event(uint32_t raw_x, uint32_t raw_y, uint8_t buttons) {
    // If raw coordinates arrive as HID normalized (0-32767), scale to current screen resolution.
    // If they arrive direct as screen pixels (< 4096), pass directly.
    int32_t abs_x = (int32_t)raw_x;
    int32_t abs_y = (int32_t)raw_y;
    
    if (raw_x > 4096 || raw_y > 4096) {
        abs_x = (int32_t)((raw_x * g_kernel_screen_width) / 32768);
        abs_y = (int32_t)((raw_y * g_kernel_screen_height) / 32768);
    }
    
    // Directly push absolute truth to BOMOUSETABUNDER abstraction normalizer
    input_push_absolute(abs_x, abs_y, buttons, 0);
}
