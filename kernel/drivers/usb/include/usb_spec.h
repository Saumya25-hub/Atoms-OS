#ifndef ATOMS_USB_SPEC_H
#define ATOMS_USB_SPEC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Standard Setup Request Direction & Recipient Flags
#define USB_REQ_DIR_OUT             0x00
#define USB_REQ_DIR_IN              0x80

#define USB_REQ_TYPE_STANDARD       0x00
#define USB_REQ_TYPE_CLASS          0x20
#define USB_REQ_TYPE_VENDOR         0x40

#define USB_REQ_REC_DEVICE          0x00
#define USB_REQ_REC_INTERFACE       0x01
#define USB_REQ_REC_ENDPOINT        0x02
#define USB_REQ_REC_OTHER          0x03

// Standard USB Requests
#define USB_REQ_GET_STATUS          0
#define USB_REQ_CLEAR_FEATURE       1
#define USB_REQ_SET_FEATURE         3
#define USB_REQ_SET_ADDRESS         5
#define USB_REQ_GET_DESCRIPTOR      6
#define USB_REQ_SET_DESCRIPTOR      7
#define USB_REQ_GET_CONFIGURATION   8
#define USB_REQ_SET_CONFIGURATION   9
#define USB_REQ_GET_INTERFACE       10
#define USB_REQ_SET_INTERFACE       11
#define USB_REQ_SYNCH_FRAME         12

// Standard Descriptor Types
#define USB_DESC_DEVICE             1
#define USB_DESC_CONFIGURATION      2
#define USB_DESC_STRING             3
#define USB_DESC_INTERFACE         4
#define USB_DESC_ENDPOINT          5
#define USB_DESC_DEVICE_QUALIFIER   6
#define USB_DESC_OTHER_SPEED_CONFIG 7
#define USB_DESC_INTERFACE_POWER    8
#define USB_DESC_OTG                9
#define USB_DESC_DEBUG              10
#define USB_DESC_INTERFACE_ASSOC    11

// HID Class Descriptor Types
#define USB_DESC_HID                0x21
#define USB_DESC_HID_REPORT         0x22
#define USB_DESC_HID_PHYSICAL       0x23

// Hub Class Descriptor Types
#define USB_DESC_HUB                0x29
#define USB_DESC_HUB_SS             0x2A

// Device Speeds
typedef enum {
    USB_SPEED_UNKNOWN = 0,
    USB_SPEED_LOW,      // 1.5 Mbps
    USB_SPEED_FULL,     // 12 Mbps
    USB_SPEED_HIGH,     // 480 Mbps
    USB_SPEED_SUPER,    // 5 Gbps
    USB_SPEED_SUPER_PLUS// 10 Gbps+
} USBSpeed;

// Standard USB Setup Packet (8 bytes)
typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) USBSetupPacket;

// Standard USB Device Descriptor (18 bytes)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__((packed)) USBDeviceDescriptor;

// Standard USB Configuration Descriptor Header (9 bytes)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} __attribute__((packed)) USBConfigurationDescriptor;

// Standard USB Interface Descriptor (9 bytes)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bInterfaceNumber;
    uint8_t  bAlternateSetting;
    uint8_t  bNumEndpoints;
    uint8_t  bInterfaceClass;
    uint8_t  bInterfaceSubClass;
    uint8_t  bInterfaceProtocol;
    uint8_t  iInterface;
} __attribute__((packed)) USBInterfaceDescriptor;

// Standard USB Endpoint Descriptor (7 bytes)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __attribute__((packed)) USBEndpointDescriptor;

// HID Class Descriptor Header
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    uint8_t  bReportDescriptorType;
    uint16_t wReportDescriptorLength;
} __attribute__((packed)) USBHIDDescriptor;

#endif // ATOMS_USB_SPEC_H
