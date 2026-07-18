#include "usb_core.h"
#include "kernel/drivers/usb/host/xhci/xhci.h"

// Forward to xHCI for now. A truly generic stack would have a HostController struct with function pointers.
// Since ATOMS OS currently only targets xHCI, we wire it directly but expose a generic API to Class Drivers.

extern bool xhci_control_transfer(USBDevice* dev, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length, void* data);
extern bool xhci_interrupt_in_transfer(USBDevice* dev, uint8_t ep_num, uint16_t max_packet_size, void* buffer, uint32_t length);

bool usb_control_transfer(USBDevice* dev, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length, void* data) {
    return xhci_control_transfer(dev, request_type, request, value, index, length, data);
}

bool usb_interrupt_in_transfer(USBDevice* dev, uint8_t ep_num, uint16_t max_packet_size, void* buffer, uint32_t length) {
    return xhci_interrupt_in_transfer(dev, ep_num, max_packet_size, buffer, length);
}
