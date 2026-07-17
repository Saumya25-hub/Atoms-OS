#include "usb_tablet.h"
#include "kernel/drivers/input/input_abstraction.h"
#include "kernel/drivers/display/display.h"
#include "kernel/drivers/input/core/hida.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

void usb_tablet_init(void) {
    display_print("[USB-TABLET] Primary Absolute Input Driver Initialized.\n");
}

void usb_tablet_report_event(uint32_t raw_x, uint32_t raw_y, uint8_t buttons) {
    // Send raw tablet coordinates (0 to 32767) directly to HIDA.
    // CCTE handles decoupling device resolution from screen resolution.
    hida_push_absolute(HIDA_BACKEND_USB, (int32_t)raw_x, (int32_t)raw_y, 32767, 32767, buttons, 0);
}
