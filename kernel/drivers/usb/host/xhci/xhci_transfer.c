#include "kernel/drivers/usb/core/usb_core.h"
#include "xhci.h"

bool xhci_control_transfer(USBDevice* dev, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length, void* data) {
    // Stub
    return true;
}

bool xhci_interrupt_in_transfer(USBDevice* dev, uint8_t ep_num, uint16_t max_packet_size, void* buffer, uint32_t length) {
    // Stub
    return true;
}
