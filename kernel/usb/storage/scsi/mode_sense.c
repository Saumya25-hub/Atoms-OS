#include "../include/usb_scsi.h"
#include "kernel/core/lib/include/string.h"

void scsi_build_mode_sense6_cdb(uint8_t* cdb, uint8_t page_code, uint8_t alloc_len) {
    if (!cdb) return;
    memset(cdb, 0, 6);
    cdb[0] = SCSI_CMD_MODE_SENSE_6;
    cdb[2] = page_code & 0x3F;
    cdb[4] = alloc_len;
}
