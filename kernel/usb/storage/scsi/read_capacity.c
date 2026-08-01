#include "../include/usb_scsi.h"
#include "kernel/core/lib/include/string.h"

void scsi_build_read_capacity_cdb(uint8_t* cdb) {
    if (!cdb) return;
    memset(cdb, 0, 10);
    cdb[0] = SCSI_CMD_READ_CAPACITY_10;
}
