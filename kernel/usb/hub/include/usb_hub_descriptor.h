#ifndef SIGNATURES_USB_HUB_DESCRIPTOR_H
#define SIGNATURES_USB_HUB_DESCRIPTOR_H

#include "../../common/usb_common.h"

// USB Hub Class Request Types
#define USB_REQUEST_GET_HUB_DESCRIPTOR   0x06
#define USB_REQUEST_GET_HUB_STATUS       0x00
#define USB_REQUEST_SET_HUB_FEATURE      0x03
#define USB_REQUEST_CLEAR_HUB_FEATURE    0x01

// Hub Descriptor Types
#define USB_DESCRIPTOR_TYPE_HUB          0x29
#define USB_DESCRIPTOR_TYPE_SS_HUB       0x2A

// Hub Feature Selectors
#define HUB_FEATURE_C_HUB_LOCAL_POWER    0
#define HUB_FEATURE_C_HUB_OVER_CURRENT   1

// Standard USB 1.1/2.0 Hub Descriptor (packed)
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bNbrPorts;
    uint16_t wHubCharacteristics;
    uint8_t  bPwrOn2PwrGood;    // in 2ms units
    uint8_t  bHubContrCurrent;  // in mA
    uint8_t  DeviceRemovable;   // bitmap
    uint8_t  PortPwrCtrlMask;   // bitmap
} usb_hub_descriptor_t;

// USB 3.0 SuperSpeed Hub Descriptor (packed)
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bNbrPorts;
    uint16_t wHubCharacteristics;
    uint8_t  bPwrOn2PwrGood;
    uint8_t  bHubContrCurrent;
    uint8_t  bHubHdrDecLat;
    uint16_t wHubDelay;
    uint16_t DeviceRemovable;
} usb_ss_hub_descriptor_t;

// Hub Characteristics Bits
#define HUB_CHAR_POWER_MASK              0x0003
#define HUB_CHAR_GANGED_POWER            0x0000
#define HUB_CHAR_INDIVIDUAL_POWER        0x0001
#define HUB_CHAR_NO_POWER_SWITCH         0x0002

#define HUB_CHAR_COMPOUND_DEV            0x0004

#define HUB_CHAR_OVERCURRENT_MASK        0x0018
#define HUB_CHAR_GLOBAL_OVERCURRENT      0x0000
#define HUB_CHAR_INDIVIDUAL_OVERCURRENT  0x0008
#define HUB_CHAR_NO_OVERCURRENT          0x0010

// API
bool usb_parse_hub_descriptor(const uint8_t* buffer, uint16_t length, usb_hub_descriptor_t* out_desc);

#endif // SIGNATURES_USB_HUB_DESCRIPTOR_H
