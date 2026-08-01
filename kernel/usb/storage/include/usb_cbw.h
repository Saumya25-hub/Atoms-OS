#ifndef SIGNATURES_USB_CBW_H
#define SIGNATURES_USB_CBW_H

#include "../../common/usb_common.h"

#define USB_CBW_SIGNATURE 0x43425355 // "USBC" in Little Endian

#define CBW_FLAGS_DATA_IN   0x80 // Device-to-Host
#define CBW_FLAGS_DATA_OUT  0x00 // Host-to-Device

// Command Block Wrapper (31 bytes packed)
typedef struct __attribute__((packed)) {
    uint32_t dCBWSignature;          // Byte 0-3: 0x43425355
    uint32_t dCBWTag;                // Byte 4-7: Unique command tag
    uint32_t dCBWDataTransferLength; // Byte 8-11: Expected bytes in data stage
    uint8_t  bmCBWFlags;             // Byte 12: Direction flag (0x80 IN, 0x00 OUT)
    uint8_t  bCBWLUN;                // Byte 13: Logical Unit Number (bits 0-3)
    uint8_t  bCBWCBLength;           // Byte 14: Length of valid SCSI CDB (1-16)
    uint8_t  CBWCB[16];              // Byte 15-30: SCSI Command Descriptor Block
} usb_cbw_t;

// API
void usb_cbw_init(usb_cbw_t* cbw, uint32_t tag, uint32_t transfer_length, uint8_t flags, uint8_t lun, uint8_t cdb_length, const uint8_t* cdb);

#endif // SIGNATURES_USB_CBW_H
