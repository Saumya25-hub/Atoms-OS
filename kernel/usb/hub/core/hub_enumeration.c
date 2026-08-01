#include "../include/usb_hub.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

bool usb_parse_hub_descriptor(const uint8_t* buffer, uint16_t length, usb_hub_descriptor_t* out_desc) {
    if (!buffer || !out_desc || length < 7) return false;
    
    out_desc->bLength = buffer[0];
    out_desc->bDescriptorType = buffer[1];
    out_desc->bNbrPorts = buffer[2];
    out_desc->wHubCharacteristics = (uint16_t)buffer[3] | ((uint16_t)buffer[4] << 8);
    out_desc->bPwrOn2PwrGood = buffer[5];
    out_desc->bHubContrCurrent = buffer[6];
    if (length >= 8) out_desc->DeviceRemovable = buffer[7];
    if (length >= 9) out_desc->PortPwrCtrlMask = buffer[8];
    
    return (out_desc->bDescriptorType == USB_DESCRIPTOR_TYPE_HUB || out_desc->bDescriptorType == USB_DESCRIPTOR_TYPE_SS_HUB);
}

usb_hub_t* usb_hub_enumerate(uint32_t device_id, uint32_t controller_id, bool is_root_hub, uint8_t num_ports) {
    display_print("[UHE ENUMERATION] Enumerating Hub (Device #");
    display_print_dec(device_id);
    display_print(")\n");
    
    usb_hub_t* hub = usb_hub_create(device_id, controller_id, is_root_hub, num_ports);
    if (!hub) return NULL;
    
    // Parse simulated/actual descriptor
    uint8_t desc_buf[9] = { 9, USB_DESCRIPTOR_TYPE_HUB, num_ports, HUB_CHAR_INDIVIDUAL_POWER, 50, 100, 0, 0 };
    usb_parse_hub_descriptor(desc_buf, 9, &hub->descriptor);
    
    // Initialize power budget
    uint32_t total_ma = (uint32_t)num_ports * USB20_PORT_POWER_LIMIT_MA;
    usb_hub_power_budget_init(&hub->power_budget, total_ma);
    
    // Initialize ports
    for (uint8_t p = 0; p < num_ports; p++) {
        usb_port_init(&hub->ports[p], p + 1);
        // Power on port
        hub->ports[p].state = PORT_STATE_POWERED;
        hub->ports[p].status |= PORT_STAT_POWER;
    }
    
    // Topology node
    hub->topology_node = usb_topology_create_node(device_id, true, 0, 0, controller_id, USB_SPEED_HIGH, 1);
    
    hub->state = HUB_STATE_RUNNING;
    usb_hub_register(hub);
    
    display_print("[UHE ENUMERATION] Hub Enumerated Successfully (ID: ");
    display_print_dec(hub->hub_id);
    display_print(")\n");
    
    return hub;
}
