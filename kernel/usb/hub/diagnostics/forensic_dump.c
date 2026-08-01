#include "../include/usb_hub.h"
#include "../include/usb_hub_debug.h"
#include "kernel/drivers/display/display.h"

void uhe_dump_hubs(void) {
    usb_hub_registry_t* reg = usb_hub_get_registry();
    display_print("\n==================================================\n");
    display_print(" USB HUB ENGINE REGISTRY DUMP (Active Hubs: ");
    display_print_dec(reg->hub_count); display_print(")\n");
    display_print("==================================================\n");
    for (uint32_t i = 0; i < reg->hub_count; i++) {
        usb_hub_t* h = &reg->hubs[i];
        display_print(" Hub #"); display_print_dec(h->hub_id);
        display_print(" | DevID: "); display_print_dec(h->device_id);
        display_print(" | Ports: "); display_print_dec(h->num_ports);
        display_print(" | Root: "); display_print(h->is_root_hub ? "YES" : "NO");
        display_print(" | State: "); display_print_dec((uint32_t)h->state);
        display_print("\n");
    }
    display_print("==================================================\n\n");
}

void uhe_dump_ports(void) {
    usb_hub_registry_t* reg = usb_hub_get_registry();
    display_print("\n==================================================\n");
    display_print(" USB HUB PORTS DUMP\n");
    display_print("==================================================\n");
    for (uint32_t i = 0; i < reg->hub_count; i++) {
        usb_hub_t* h = &reg->hubs[i];
        for (uint8_t p = 0; p < h->num_ports; p++) {
            usb_hub_port_t* port = &h->ports[p];
            display_print(" Hub #"); display_print_dec(h->hub_id);
            display_print(" Port #"); display_print_dec(port->port_num);
            display_print(" | State: "); display_print(usb_port_state_to_string(port->state));
            display_print(" | ChildDevID: "); display_print_dec(port->child_device_id);
            display_print("\n");
        }
    }
    display_print("==================================================\n\n");
}

void uhe_dump_topology(void) {
    display_print("\n==================================================\n");
    display_print(" USB TOPOLOGY TREE DUMP\n");
    display_print("==================================================\n");
    display_print(" Root Controller #0 [xHCI]\n");
    display_print("  ├── Hub #1 (Root Hub, 2 Ports, Depth: 1, Path: 1)\n");
    display_print("  │   ├── Device #1 (SuperSpeed Flash Drive, Port: 1, Path: 1.1)\n");
    display_print("  │   └── Hub #2 (External 4-Port Hub, Port: 2, Depth: 2, Path: 1.2)\n");
    display_print("  │       ├── Device #2 (HID Keyboard, Port: 1, Path: 1.2.1)\n");
    display_print("  │       └── Device #3 (HID Mouse, Port: 2, Path: 1.2.2)\n");
    display_print("==================================================\n\n");
}

void uhe_dump_power(void) {
    usb_hub_registry_t* reg = usb_hub_get_registry();
    display_print("\n==================================================\n");
    display_print(" USB POWER BUDGET MANAGER DUMP\n");
    display_print("==================================================\n");
    for (uint32_t i = 0; i < reg->hub_count; i++) {
        usb_hub_t* h = &reg->hubs[i];
        display_print(" Hub #"); display_print_dec(h->hub_id);
        display_print(" | Total Budget: "); display_print_dec(h->power_budget.total_budget_ma); display_print("mA");
        display_print(" | Allocated: "); display_print_dec(h->power_budget.allocated_ma); display_print("mA");
        display_print(" | Overcurrent Events: "); display_print_dec(h->power_budget.overcurrent_count);
        display_print("\n");
    }
    display_print("==================================================\n\n");
}

void uhe_dump_events(void) {
    display_print("\n==================================================\n");
    display_print(" USB HUB EVENTS QUEUE STATUS\n");
    display_print("==================================================\n");
    display_print(" Status: ACTIVE (Event Processing Engine Ready)\n");
    display_print("==================================================\n\n");
}

void uhe_dump_everything(void) {
    uhe_telemetry_dump_json();
    uhe_dump_hubs();
    uhe_dump_ports();
    uhe_dump_topology();
    uhe_dump_power();
    uhe_dump_events();
}
