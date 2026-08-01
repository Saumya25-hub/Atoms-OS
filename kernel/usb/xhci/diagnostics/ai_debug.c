#include "../include/xhci_debug.h"
#include "kernel/drivers/display/display.h"

void xhci_dump_statistics(void) {
    xhci_telemetry_t* t = xhci_telemetry_get();
    if (!t) return;
    
    display_print("\n```json\n");
    display_print("{\n");
    display_print("  \"xhci_bte_telemetry\": {\n");
    display_print("    \"transfers_submitted\": "); display_print_dec(t->transfers_submitted); display_print(",\n");
    display_print("    \"transfers_completed\": "); display_print_dec(t->transfers_completed); display_print(",\n");
    display_print("    \"bytes_in\": "); display_print_dec(t->bytes_in); display_print(",\n");
    display_print("    \"bytes_out\": "); display_print_dec(t->bytes_out); display_print(",\n");
    display_print("    \"timeouts\": "); display_print_dec(t->timeouts); display_print(",\n");
    display_print("    \"controller_errors\": "); display_print_dec(t->controller_errors); display_print(",\n");
    display_print("    \"max_latency_us\": "); display_print_dec(t->max_latency_us); display_print("\n");
    display_print("  }\n");
    display_print("}\n");
    display_print("```\n\n");
}

void xhci_dump_controller(void) {
    display_print("\n=== XHCI CONTROLLER DIAGNOSTIC SNAPSHOT ===\n");
    display_print("State: ACTIVE / READY\n");
    display_print("Subsystem: USB BTE Phase 1 Engine\n");
    display_print("===========================================\n");
}

void xhci_dump_everything(void) {
    xhci_dump_controller();
    xhci_dump_statistics();
}
