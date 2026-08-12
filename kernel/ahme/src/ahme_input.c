#include "kernel/ahme/include/ahme_input.h"
#include "kernel/ahme/include/ahme_quirks.h"

static AHMEInputRouteMode s_current_route = AHME_INPUT_ROUTE_FALLBACK_HYBRID;
static uint64_t s_irq12_ticks = 0;
static uint64_t s_usb_hid_ticks = 0;

void ahme_input_init(void) {
    if (ahme_quirks_has(AHME_QUIRK_PREFER_USB_HID_INPUT)) {
        s_current_route = AHME_INPUT_ROUTE_USB_HID_ACTIVE;
    } else {
        s_current_route = AHME_INPUT_ROUTE_PS2_ACTIVE;
    }
}

AHMEInputRouteMode ahme_input_get_route(void) {
    return s_current_route;
}

void ahme_input_notify_irq12_activity(void) {
    s_irq12_ticks++;
    if (s_current_route == AHME_INPUT_ROUTE_FALLBACK_HYBRID && s_irq12_ticks > 5) {
        s_current_route = AHME_INPUT_ROUTE_PS2_ACTIVE;
    }
}

void ahme_input_notify_usb_hid_activity(void) {
    s_usb_hid_ticks++;
    s_current_route = AHME_INPUT_ROUTE_USB_HID_ACTIVE;
}
