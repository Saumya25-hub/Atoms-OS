#include "usb_registry.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

#define MAX_USB_CLASS_DRIVERS 32
static USBClassDriver g_class_drivers[MAX_USB_CLASS_DRIVERS];
static uint32_t g_driver_count = 0;

void usb_registry_init(void) {
    memset(g_class_drivers, 0, sizeof(g_class_drivers));
    g_driver_count = 0;
}

void usb_register_class_driver(USBClassDriver driver) {
    if (g_driver_count >= MAX_USB_CLASS_DRIVERS) {
        display_print("[USB REGISTRY] Error: Max class drivers reached\n");
        return;
    }
    g_class_drivers[g_driver_count++] = driver;
    display_print("[USB REGISTRY] Registered Driver: ");
    display_print(driver.name);
    display_print("\n");
}

bool usb_bind_drivers(USBDevice* dev, void* config_desc_buffer, uint16_t total_length) {
    uint8_t* ptr = (uint8_t*)config_desc_buffer;
    uint8_t* end = ptr + total_length;
    
    bool bound_any = false;
    
    while (ptr < end) {
        USBDescriptorHeader* hdr = (USBDescriptorHeader*)ptr;
        if (hdr->bLength == 0) break; // Avoid infinite loop on malformed descriptor
        
        if (hdr->bDescriptorType == USB_DESC_INTERFACE) {
            USBInterfaceDescriptor* iface = (USBInterfaceDescriptor*)ptr;
            
            // Try to match drivers
            for (uint32_t i = 0; i < g_driver_count; i++) {
                USBClassDriver* drv = &g_class_drivers[i];
                if (drv->class_code == iface->bInterfaceClass &&
                    (drv->subclass_code == 0xFF || drv->subclass_code == iface->bInterfaceSubClass) &&
                    (drv->protocol_code == 0xFF || drv->protocol_code == iface->bInterfaceProtocol)) {
                    
                    display_print("[USB REGISTRY] Binding interface to ");
                    display_print(drv->name);
                    display_print("\n");
                    
                    if (drv->bind(dev, iface, config_desc_buffer, total_length)) {
                        bound_any = true;
                        // For a real generic stack we'd register endpoints here and associate interface data,
                        // but for now the bind callback does what it needs.
                    }
                }
            }
        }
        
        ptr += hdr->bLength;
    }
    
    return bound_any;
}
