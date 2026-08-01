#include "../include/usb_scsi.h"
#include "kernel/core/lib/include/string.h"

void scsi_build_inquiry_cdb(uint8_t* cdb, uint8_t alloc_len) {
    if (!cdb) return;
    memset(cdb, 0, 6);
    cdb[0] = SCSI_CMD_INQUIRY;
    cdb[4] = alloc_len;
}
