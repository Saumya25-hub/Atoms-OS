#ifndef SIGNATURES_USB_REGISTRY_H
#define SIGNATURES_USB_REGISTRY_H

#include "usb_core.h"

// Interface Descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed)) USBInterfaceDescriptor;

typedef struct {
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t protocol_code;
    // Callback when a matching interface is found
    bool (*bind)(USBDevice* dev, USBInterfaceDescriptor* interface_desc, void* config_desc_buffer, uint16_t total_length);
    const char* name;
} USBClassDriver;

void usb_registry_init(void);
void usb_register_class_driver(USBClassDriver driver);
bool usb_bind_drivers(USBDevice* dev, void* config_desc_buffer, uint16_t total_length);

#endif // SIGNATURES_USB_REGISTRY_H
