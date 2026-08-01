#ifndef SIGNATURES_USB_SCSI_H
#define SIGNATURES_USB_SCSI_H

#include "../../common/usb_common.h"
#include "usb_storage_errors.h"

// SCSI Opcodes
#define SCSI_CMD_TEST_UNIT_READY  0x00
#define SCSI_CMD_REQUEST_SENSE    0x03
#define SCSI_CMD_INQUIRY          0x12
#define SCSI_CMD_MODE_SENSE_6     0x1A
#define SCSI_CMD_START_STOP_UNIT  0x1B
#define SCSI_CMD_READ_CAPACITY_10 0x25
#define SCSI_CMD_READ_10          0x28
#define SCSI_CMD_WRITE_10         0x2A
#define SCSI_CMD_VERIFY_10        0x2F

// Standard INQUIRY Data (36 bytes packed)
typedef struct __attribute__((packed)) {
    uint8_t  peripheral_device_type; // bits 0-4: 0x00 Direct-access block device
    uint8_t  rmb;                    // bit 7: Removable Medium Bit
    uint8_t  version;
    uint8_t  response_data_format;
    uint8_t  additional_length;      // 31
    uint8_t  flags[3];
    char     vendor_id[8];
    char     product_id[16];
    char     product_rev[4];
} scsi_inquiry_data_t;

// READ CAPACITY(10) Data (8 bytes packed)
typedef struct __attribute__((packed)) {
    uint32_t last_lba;    // Big Endian
    uint32_t block_size;  // Big Endian (e.g. 512)
} scsi_read_capacity_data_t;

// API
void scsi_build_inquiry_cdb(uint8_t* cdb, uint8_t alloc_len);
void scsi_build_read_capacity_cdb(uint8_t* cdb);
void scsi_build_test_unit_ready_cdb(uint8_t* cdb);
void scsi_build_request_sense_cdb(uint8_t* cdb, uint8_t alloc_len);
void scsi_build_read10_cdb(uint8_t* cdb, uint32_t lba, uint16_t blocks);
void scsi_build_write10_cdb(uint8_t* cdb, uint32_t lba, uint16_t blocks);
void scsi_build_mode_sense6_cdb(uint8_t* cdb, uint8_t page_code, uint8_t alloc_len);

#endif // SIGNATURES_USB_SCSI_H
