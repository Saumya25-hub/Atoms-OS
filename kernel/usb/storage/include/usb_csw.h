#ifndef SIGNATURES_USB_CSW_H
#define SIGNATURES_USB_CSW_H

#include "../../common/usb_common.h"

#define USB_CSW_SIGNATURE 0x53425355 // "USBS" in Little Endian

typedef enum {
    CSW_STATUS_PASSED      = 0x00, // Command Completed Successfully
    CSW_STATUS_FAILED      = 0x01, // Command Failed
    CSW_STATUS_PHASE_ERROR = 0x02  // Phase Error (Requires Reset Recovery)
} usb_csw_status_t;

// Command Status Wrapper (13 bytes packed)
typedef struct __attribute__((packed)) {
    uint32_t dCSWSignature;   // Byte 0-3: 0x53425355
    uint32_t dCSWTag;         // Byte 4-7: Matches dCBWTag
    uint32_t dCSWDataResidue; // Byte 8-11: Difference between expected and actual data bytes
    uint8_t  bCSWStatus;      // Byte 12: Status code
} usb_csw_t;

// API
bool usb_csw_validate(const usb_csw_t* csw, uint32_t expected_tag);

#endif // SIGNATURES_USB_CSW_H
