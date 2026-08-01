#include "../include/usb_scsi.h"
#include "kernel/core/lib/include/string.h"

void scsi_build_test_unit_ready_cdb(uint8_t* cdb) {
    if (!cdb) return;
    memset(cdb, 0, 6);
    cdb[0] = SCSI_CMD_TEST_UNIT_READY;
}
