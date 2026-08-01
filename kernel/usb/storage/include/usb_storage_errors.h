#ifndef SIGNATURES_USB_STORAGE_ERRORS_H
#define SIGNATURES_USB_STORAGE_ERRORS_H

#include "../../common/usb_common.h"

// SCSI Sense Keys (SPC-3)
typedef enum {
    SCSI_SENSE_NO_SENSE         = 0x0,
    SCSI_SENSE_RECOVERED_ERROR  = 0x1,
    SCSI_SENSE_NOT_READY        = 0x2,
    SCSI_SENSE_MEDIUM_ERROR     = 0x3,
    SCSI_SENSE_HARDWARE_ERROR   = 0x4,
    SCSI_SENSE_ILLEGAL_REQUEST  = 0x5,
    SCSI_SENSE_UNIT_ATTENTION   = 0x6,
    SCSI_SENSE_DATA_PROTECT     = 0x7,
    SCSI_SENSE_BLANK_CHECK      = 0x8,
    SCSI_SENSE_VENDOR_SPECIFIC  = 0x9,
    SCSI_SENSE_ABORTED_COMMAND  = 0xB,
    SCSI_SENSE_VOLUME_OVERFLOW  = 0xD,
    SCSI_SENSE_MISCOMPARE       = 0xE
} scsi_sense_key_t;

// Fixed Format Sense Data (18 bytes packed)
typedef struct __attribute__((packed)) {
    uint8_t  response_code;      // 0x70 or 0x71
    uint8_t  obsolete;
    uint8_t  sense_key;          // Sense Key in bits 0-3
    uint32_t information;
    uint8_t  additional_sense_len; // 10
    uint32_t command_specific_info;
    uint8_t  asc;                // Additional Sense Code
    uint8_t  ascq;               // Additional Sense Code Qualifier
    uint8_t  fruc;
    uint8_t  sense_key_specific[3];
} scsi_sense_data_t;

// API
const char* scsi_sense_key_to_string(scsi_sense_key_t key);

#endif // SIGNATURES_USB_STORAGE_ERRORS_H
