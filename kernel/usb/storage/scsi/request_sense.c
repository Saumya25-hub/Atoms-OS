#include "../include/usb_scsi.h"
#include "kernel/core/lib/include/string.h"

void scsi_build_request_sense_cdb(uint8_t* cdb, uint8_t alloc_len) {
    if (!cdb) return;
    memset(cdb, 0, 6);
    cdb[0] = SCSI_CMD_REQUEST_SENSE;
    cdb[4] = alloc_len;
}

const char* scsi_sense_key_to_string(scsi_sense_key_t key) {
    switch (key) {
        case SCSI_SENSE_NO_SENSE:        return "NO SENSE";
        case SCSI_SENSE_RECOVERED_ERROR: return "RECOVERED ERROR";
        case SCSI_SENSE_NOT_READY:       return "NOT READY";
        case SCSI_SENSE_MEDIUM_ERROR:    return "MEDIUM ERROR";
        case SCSI_SENSE_HARDWARE_ERROR:  return "HARDWARE ERROR";
        case SCSI_SENSE_ILLEGAL_REQUEST: return "ILLEGAL REQUEST";
        case SCSI_SENSE_UNIT_ATTENTION:  return "UNIT ATTENTION";
        case SCSI_SENSE_DATA_PROTECT:    return "DATA PROTECT";
        case SCSI_SENSE_ABORTED_COMMAND: return "ABORTED COMMAND";
        default: return "UNKNOWN SENSE KEY";
    }
}
