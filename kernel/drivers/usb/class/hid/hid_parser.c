#include "kernel/drivers/usb/include/usb_spec.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    uint8_t report_id;
    uint16_t usage_page;
    uint16_t usage;
    uint32_t logical_min;
    uint32_t logical_max;
    uint16_t report_size;
    uint16_t report_count;
} HIDReportItem;

bool hid_parse_report_descriptor(const uint8_t* desc, size_t desc_len) {
    if (!desc || desc_len == 0) return false;

    // Fast-path sanity check for HID Mouse/Keyboard report descriptor tags
    bool found_pointer_or_mouse = false;
    for (size_t i = 0; i < desc_len; i++) {
        if (desc[i] == 0x05 && (i + 1 < desc_len) && desc[i+1] == 0x01) { // Generic Desktop Page
            if (i + 3 < desc_len && desc[i+2] == 0x09) {
                if (desc[i+3] == 0x02 || desc[i+3] == 0x01 || desc[i+3] == 0x06) { // Mouse / Pointer / Keyboard
                    found_pointer_or_mouse = true;
                    break;
                }
            }
        }
    }
    return found_pointer_or_mouse;
}
