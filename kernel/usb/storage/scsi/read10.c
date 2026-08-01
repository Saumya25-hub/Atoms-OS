#include "../include/usb_scsi.h"
#include "kernel/core/lib/include/string.h"

void scsi_build_read10_cdb(uint8_t* cdb, uint32_t lba, uint16_t blocks) {
    if (!cdb) return;
    memset(cdb, 0, 10);
    cdb[0] = SCSI_CMD_READ_10;
    cdb[2] = (lba >> 24) & 0xFF;
    cdb[3] = (lba >> 16) & 0xFF;
    cdb[4] = (lba >> 8) & 0xFF;
    cdb[5] = lba & 0xFF;
    cdb[7] = (blocks >> 8) & 0xFF;
    cdb[8] = blocks & 0xFF;
}
