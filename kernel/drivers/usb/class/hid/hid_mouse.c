#include "kernel/drivers/usb/include/usb_spec.h"
#include "kernel/core/lib/include/string.h"

extern volatile uint64_t g_mouse_events;

void hid_mouse_handle_report(const uint8_t* report, size_t len) {
    if (!report || len < 3) return;

    uint8_t buttons = report[0];
    int8_t dx = (int8_t)report[1];
    int8_t dy = (int8_t)report[2];
    int8_t wheel = (len >= 4) ? (int8_t)report[3] : 0;

    (void)buttons; (void)dx; (void)dy; (void)wheel;

    g_mouse_events++;
}
