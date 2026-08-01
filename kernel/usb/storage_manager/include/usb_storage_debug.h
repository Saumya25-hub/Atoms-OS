#ifndef SIGNATURES_USB_STORAGE_DEBUG_PHASE6_H
#define SIGNATURES_USB_STORAGE_DEBUG_PHASE6_H

#include "../../common/usb_common.h"

// Forensic Dump APIs
void usm_dump_disks(void);
void usm_dump_partitions(void);
void usm_dump_mounts(void);
void usm_dump_volumes(void);
void usm_dump_cache(void);
void usm_dump_scheduler(void);
void usm_dump_everything(void);

// Telemetry Export API
void usm_telemetry_init(void);
void usm_telemetry_dump_json(void);

#endif // SIGNATURES_USB_STORAGE_DEBUG_PHASE6_H
