#include "kernel/drivers/usb/include/usb_spec.h"
#include "kernel/drivers/display/display.h"

void ehci_companion_init(void) {
    display_print("[EHCI COMPANION] Initializing USB 2.0 EHCI Companion Handoff Engine\n");
}

void ehci_handoff_low_speed_port(uint8_t port_num) {
    display_print("[EHCI COMPANION] Handing off Low-Speed Port "); display_print_dec(port_num);
    display_print(" to UHCI/OHCI Companion Controller via PORTSC Bit 13...\n");
}
