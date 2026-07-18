#include "usb_core.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

// External xHCI calls to manage slots and addresses (since xHCI handles this in hardware instead of SET_ADDRESS)
extern uint8_t xhci_enable_slot(void);
extern bool xhci_address_device(uint8_t slot_id, uint8_t port, uint8_t speed);

void usb_device_connected(uint8_t port, uint8_t speed) {
    display_print("[USB ENUM] Device connected on port ");
    extern void display_print_dec(uint64_t);
    display_print_dec(port);
    display_print(" Speed: ");
    display_print_dec(speed);
    display_print("\n");

    // 1. Enable Slot
    uint8_t slot_id = xhci_enable_slot();
    if (slot_id == 0) {
        display_print("[USB ENUM] Failed to enable slot\n");
        return;
    }
    
    // 2. Address Device (xHCI handles SET_ADDRESS implicitly in Address Device command)
    if (!xhci_address_device(slot_id, port, speed)) {
        display_print("[USB ENUM] Failed to address device\n");
        return;
    }
    
    USBDevice dev;
    memset(&dev, 0, sizeof(dev));
    dev.slot_id = slot_id;
    dev.port = port;
    dev.speed = speed;
    // xHCI device context maintains the address. We don't strictly need it in our abstraction, 
    // but we can query it if needed. We'll set it to slot_id for simplicity as an identifier.
    dev.address = slot_id; 

    // 3. GET_DESCRIPTOR(Device)
    USBDeviceDescriptor dev_desc;
    memset(&dev_desc, 0, sizeof(dev_desc));
    
    bool ok = usb_control_transfer(&dev, USB_REQ_TYPE_STANDARD | USB_REQ_DIR_IN | USB_REQ_REC_DEVICE,
                                   USB_REQ_GET_DESCRIPTOR, (USB_DESC_DEVICE << 8) | 0, 0,
                                   sizeof(USBDeviceDescriptor), &dev_desc);
                                   
    if (!ok) {
        display_print("[USB ENUM] Failed to get Device Descriptor\n");
        return;
    }
    
    dev.vid = dev_desc.idVendor;
    dev.pid = dev_desc.idProduct;
    dev.max_packet_size = dev_desc.bMaxPacketSize0;
    
    usb_register_device(&dev);
    
    // 4. GET_DESCRIPTOR(Configuration)
    // First get the header to know the total length
    uint8_t config_buf[256];
    memset(config_buf, 0, sizeof(config_buf));
    
    ok = usb_control_transfer(&dev, USB_REQ_TYPE_STANDARD | USB_REQ_DIR_IN | USB_REQ_REC_DEVICE,
                              USB_REQ_GET_DESCRIPTOR, (USB_DESC_CONFIGURATION << 8) | 0, 0,
                              sizeof(config_buf), config_buf);
                              
    if (!ok) {
        display_print("[USB ENUM] Failed to get Configuration Descriptor\n");
        return;
    }
    
    // 5. SET_CONFIGURATION (Config 1)
    ok = usb_control_transfer(&dev, USB_REQ_TYPE_STANDARD | USB_REQ_DIR_OUT | USB_REQ_REC_DEVICE,
                              USB_REQ_SET_CONFIGURATION, 1, 0, 0, NULL);
                              
    if (!ok) {
        display_print("[USB ENUM] Failed to set Configuration\n");
        return;
    }
    
    display_print("[USB ENUM] Configuration 1 set. Binding drivers...\n");
    
    // 6. Bind Class Drivers
    extern bool usb_bind_drivers(USBDevice* dev, void* config_desc_buffer, uint16_t total_length);
    if (!usb_bind_drivers(&dev, config_buf, sizeof(config_buf))) {
        display_print("[USB ENUM] No drivers bound to device.\n");
    }
}
