#ifndef SIGNATURES_USB_CORE_H
#define SIGNATURES_USB_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/usb/common/usb_common.h"

#define USB_REQ_TYPE_STANDARD 0x00
#define USB_REQ_TYPE_CLASS 0x20
#define USB_REQ_TYPE_VENDOR 0x40

#define USB_REQ_REC_DEVICE 0x00
#define USB_REQ_REC_INTERFACE 0x01
#define USB_REQ_REC_ENDPOINT 0x02
#define USB_REQ_REC_OTHER 0x03

#define USB_REQ_DIR_OUT 0x00
#define USB_REQ_DIR_IN 0x80

// Standard Requests
#define USB_REQ_GET_STATUS 0
#define USB_REQ_CLEAR_FEATURE 1
#define USB_REQ_SET_FEATURE 3
#define USB_REQ_SET_ADDRESS 5
#define USB_REQ_GET_DESCRIPTOR 6
#define USB_REQ_SET_DESCRIPTOR 7
#define USB_REQ_GET_CONFIGURATION 8
#define USB_REQ_SET_CONFIGURATION 9
#define USB_REQ_SET_IDLE 0x0A

// Descriptor Types
#define USB_DESC_DEVICE 1
#define USB_DESC_CONFIGURATION 2
#define USB_DESC_STRING 3
#define USB_DESC_INTERFACE 4
#define USB_DESC_ENDPOINT 5
#define USB_DESC_HID 0x21
#define USB_DESC_HID_REPORT 0x22

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
} __attribute__((packed)) USBDescriptorHeader;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __attribute__((packed)) USBDeviceDescriptor;

typedef struct USBDevice {
    uint8_t address;
    uint8_t port;
    uint8_t slot_id;
    uint8_t speed;
    uint16_t max_packet_size;
    uint16_t vid;
    uint16_t pid;
    uint8_t protocol; // HID Protocol: 1 = Keyboard, 2 = Mouse
    uint8_t interface_number;
    void* driver_data; // For class drivers
} USBDevice;

typedef USBDevice usb_device_t;

#include "kernel/usb/urb/usb_urb.h"

// USB Realtime Hardware Telemetry Structure
typedef struct {
    bool xhci_started;
    bool enable_slot_pass;
    bool address_device_pass;
    bool get_descriptor_pass;
    bool set_config_pass;
    bool configure_ep_pass;
    bool interrupt_in_pass;
    uint32_t mouse_packet_count;
    uint32_t keyboard_packet_count;
    uint8_t last_key_code;
    char last_key_ascii;
} USBRealtimeDiagnostics;

extern USBRealtimeDiagnostics g_usb_diag;

// Core APIs
void usb_core_init(void);
void usb_device_connected(uint8_t port, uint8_t speed);
USBDevice* usb_register_device(USBDevice* dev);

// Legacy Transfer APIs
bool usb_control_transfer(USBDevice* dev, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length, void* data);
bool usb_interrupt_in_transfer(USBDevice* dev, uint8_t ep_num, uint16_t max_packet_size, void* buffer, uint32_t length);

#endif // SIGNATURES_USB_CORE_H
