/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 1999 MAEKAWA Masahide <genta@netbsd.org>
 * Copyright (c) 2007-2008 Hans Petter Selasky <hselasky@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Adapted for ATOMS OS: Isolated standard USB MSC BOT and SCSI definitions.
 */

#ifndef _THIRD_PARTY_USB_MSC_DEFS_H_
#define _THIRD_PARTY_USB_MSC_DEFS_H_

#include <stdint.h>
#include <stdbool.h>

/* USB Mass Storage Class Specifications */
#define USB_MSC_SUBCLASS_RBC            0x01    /* Reduced Block Commands */
#define USB_MSC_SUBCLASS_ATAPI          0x02    /* CD/DVD ATAPI */
#define USB_MSC_SUBCLASS_QIC_157        0x03    /* QIC-157 Tape */
#define USB_MSC_SUBCLASS_UFI            0x04    /* Floppy UFI */
#define USB_MSC_SUBCLASS_SFF_8070I      0x05    /* SFF-8070i */
#define USB_MSC_SUBCLASS_SCSI           0x06    /* SCSI Transparent Command Set */

#define USB_MSC_PROTOCOL_CBI_INTERRUPT  0x00    /* Control/Bulk/Interrupt with completion */
#define USB_MSC_PROTOCOL_CBI_NO_INT     0x01    /* Control/Bulk/Interrupt without completion */
#define USB_MSC_PROTOCOL_BOT            0x50    /* Bulk-Only Transport */

/* Bulk-Only Transport Requests */
#define UR_BBB_RESET                    0xFF    /* Mass Storage Reset */
#define UR_BBB_GET_MAX_LUN              0xFE    /* Get Max LUN */

/* Command Block Wrapper (CBW) */
#define CBWSIGNATURE                    0x43425355U /* "USBC" */
#define CBWFLAGS_OUT                    0x00U
#define CBWFLAGS_IN                     0x80U
#define CBW_MAX_CDB_LEN                 16U

typedef struct {
    uint32_t dCBWSignature;             /* 0x43425355 */
    uint32_t dCBWTag;                   /* Unique command sequence tag */
    uint32_t dCBWDataTransferLength;   /* Number of bytes host expects to transfer */
    uint8_t  bmCBWFlags;                /* 0x80 = IN, 0x00 = OUT */
    uint8_t  bCBWLUN;                   /* Target Logical Unit Number */
    uint8_t  bCBWCBLength;              /* Length of valid CDB bytes (1..16) */
    uint8_t  CBWCB[16];                 /* Command descriptor block */
} __attribute__((packed)) usb_msc_cbw_t;

/* Command Status Wrapper (CSW) */
#define CSWSIGNATURE                    0x53425355U /* "USBS" */
#define CSWSTATUS_GOOD                  0x00U
#define CSWSTATUS_FAILED                0x01U
#define CSWSTATUS_PHASE                 0x02U

typedef struct {
    uint32_t dCSWSignature;             /* 0x53425355 */
    uint32_t dCSWTag;                   /* Must match dCBWTag */
    uint32_t dCSWDataResidue;           /* Difference between expected and actual bytes */
    uint8_t  bCSWStatus;                /* 0x00 = Good, 0x01 = Failed, 0x02 = Phase Error */
} __attribute__((packed)) usb_msc_csw_t;

/* Standard SCSI Command Opcodes */
#define SCSI_TEST_UNIT_READY            0x00U
#define SCSI_REQUEST_SENSE              0x03U
#define SCSI_INQUIRY                    0x12U
#define SCSI_MODE_SENSE_6               0x1AU
#define SCSI_START_STOP_UNIT            0x1BU
#define SCSI_PREVENT_ALLOW_MEDIUM       0x1EU
#define SCSI_READ_CAPACITY_10           0x25U
#define SCSI_READ_10                    0x28U
#define SCSI_WRITE_10                   0x2AU
#define SCSI_SYNCHRONIZE_CACHE_10       0x35U
#define SCSI_MODE_SENSE_10              0x5AU
#define SCSI_READ_16                    0x88U
#define SCSI_WRITE_16                   0x8AU
#define SCSI_READ_CAPACITY_16           0x9EU

/* SCSI Sense Keys */
#define SCSI_SENSE_NO_SENSE             0x00U
#define SCSI_SENSE_RECOVERED_ERROR      0x01U
#define SCSI_SENSE_NOT_READY            0x02U
#define SCSI_SENSE_MEDIUM_ERROR         0x03U
#define SCSI_SENSE_HARDWARE_ERROR       0x04U
#define SCSI_SENSE_ILLEGAL_REQUEST      0x05U
#define SCSI_SENSE_UNIT_ATTENTION       0x06U
#define SCSI_SENSE_DATA_PROTECT         0x07U
#define SCSI_SENSE_BLANK_CHECK          0x08U
#define SCSI_SENSE_ABORTED_COMMAND      0x0BU
#define SCSI_SENSE_VOLUME_OVERFLOW      0x0DU
#define SCSI_SENSE_MISCOMPARE           0x0EU

/* Standard SCSI Inquiry Data (36 bytes minimum) */
typedef struct {
    uint8_t peripheral_device_type : 5;
    uint8_t peripheral_qualifier   : 3;
    uint8_t reserved1              : 7;
    uint8_t rmb                    : 1; /* Removable Media Bit */
    uint8_t version;
    uint8_t response_data_format   : 4;
    uint8_t reserved2              : 4;
    uint8_t additional_length;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t flags3;
    char    vendor_id[8];
    char    product_id[16];
    char    product_rev[4];
} __attribute__((packed)) scsi_inquiry_response_t;

/* SCSI Read Capacity 10 Response (8 bytes) */
typedef struct {
    uint32_t last_lba;                  /* Big-Endian */
    uint32_t block_size;                /* Big-Endian */
} __attribute__((packed)) scsi_read_capacity_10_response_t;

/* SCSI Request Sense Response (18 bytes standard) */
typedef struct {
    uint8_t response_code;
    uint8_t segment_number;
    uint8_t sense_key : 4;
    uint8_t reserved : 4;
    uint32_t information;
    uint8_t additional_sense_length;
    uint32_t command_specific_info;
    uint8_t additional_sense_code;
    uint8_t additional_sense_code_qualifier;
    uint8_t field_replaceable_unit_code;
    uint8_t sense_key_specific[3];
} __attribute__((packed)) scsi_request_sense_response_t;

#endif /* _THIRD_PARTY_USB_MSC_DEFS_H_ */
