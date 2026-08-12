#include "kernel/drivers/usb/include/usb_spec.h"
#include "kernel/drivers/display/display.h"

void uhci_hcd_init(void) {
    display_print("[UHCI HCD] Initializing Native Intel UHCI (USB 1.1) Host Controller Driver\n");
}
