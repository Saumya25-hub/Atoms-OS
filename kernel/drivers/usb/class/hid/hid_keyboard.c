#include "kernel/drivers/usb/include/usb_spec.h"

extern volatile uint64_t g_keyboard_events;

void hid_keyboard_handle_report(const uint8_t* report, size_t len) {
    if (!report || len < 8) return;

    uint8_t modifiers = report[0];
    (void)modifiers;

    for (size_t i = 2; i < 8; i++) {
        if (report[i] != 0) {
            g_keyboard_events++;
        }
    }
}
