#ifndef SIGNATURES_USB_STORAGE_DEBUG_H
#define SIGNATURES_USB_STORAGE_DEBUG_H

#include "../../common/usb_common.h"

// Forensic Dump APIs
void ums_dump_storage(void);
void ums_dump_scsi(void);
void ums_dump_bot(void);
void ums_dump_cbw(void);
void ums_dump_csw(void);
void ums_dump_lba(void);
void ums_dump_everything(void);

// Telemetry Export API
void ums_telemetry_init(void);
void ums_telemetry_dump_json(void);

#endif // SIGNATURES_USB_STORAGE_DEBUG_H
