#include "kernel/drivers/usb/include/usb_spec.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

void hid_core_init(void) {
    display_print("[USB HID CORE] Initializing Universal USB HID Class Driver Library\n");
}

bool hid_process_packet(const uint8_t* data, size_t len, bool is_mouse) {
    if (!data || len == 0) return false;

    if (is_mouse) {
        extern void hid_mouse_handle_report(const uint8_t* report, size_t len);
        hid_mouse_handle_report(data, len);
        return true;
    } else {
        extern void hid_keyboard_handle_report(const uint8_t* report, size_t len);
        hid_keyboard_handle_report(data, len);
        return true;
    }
}
