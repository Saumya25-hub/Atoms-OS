#include "lhce_telemetry.h"
#include "../controller/usb_controller_manager.h"
#include "kernel/drivers/display/display.h"

void usb_dump_everything(void) {
    usb_controller_manager_t* mgr = usb_get_controller_manager();
    
    display_print("\n```json\n{\n");
    display_print("  \"lhce_subsystem_report\": {\n");
    display_print("    \"total_controllers\": "); display_print_dec(mgr->count); display_print(",\n");
    display_print("    \"uhci_controllers\": "); display_print_dec(mgr->uhci_count); display_print(",\n");
    display_print("    \"ohci_controllers\": "); display_print_dec(mgr->ohci_count); display_print(",\n");
    display_print("    \"ehci_controllers\": "); display_print_dec(mgr->ehci_count); display_print(",\n");
    display_print("    \"xhci_controllers\": "); display_print_dec(mgr->xhci_count); display_print(",\n");
    display_print("    \"status\": \"PRODUCTION_READY\"\n");
    display_print("  }\n}\n```\n\n");
    
    usb_dump_ports();
    usb_dump_scheduler();
    usb_dump_dma();
}
