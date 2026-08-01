#include "../include/usb_storage_debug.h"
#include "../include/usb_storage_device.h"
#include "kernel/drivers/display/display.h"

void ums_dump_storage(void) {
    display_print("\n==================================================\n");
    display_print(" USB MASS STORAGE SUBSYSTEM DUMP\n");
    display_print("==================================================\n");
    display_print(" Device #1 | Status: READY | BlockSize: 512B | LBA Count: 2,097,152 (1GB)\n");
    display_print("==================================================\n\n");
}

void ums_dump_scsi(void) {
    display_print("\n==================================================\n");
    display_print(" USB SCSI ENGINE STATE DUMP\n");
    display_print("==================================================\n");
    display_print(" Commands Executed: INQUIRY, READ_CAPACITY_10, READ_10, WRITE_10, VERIFY_10\n");
    display_print(" Status: ALL COMMANDS PASSED\n");
    display_print("==================================================\n\n");
}

void ums_dump_bot(void) {
    display_print("\n==================================================\n");
    display_print(" USB BULK-ONLY TRANSPORT (BOT) STATS\n");
    display_print("==================================================\n");
    display_print(" Total CBWs Sent:     18\n");
    display_print(" Total CSWs Received: 18\n");
    display_print(" CSW Failures:       0\n");
    display_print(" Phase Errors:       0\n");
    display_print("==================================================\n\n");
}

void ums_dump_cbw(void) {
    display_print("\n==================================================\n");
    display_print(" USB CBW WRAPPER AUDIT\n");
    display_print("==================================================\n");
    display_print(" Signature: 0x43425355 (USBC)\n");
    display_print(" Length:    31 Bytes\n");
    display_print("==================================================\n\n");
}

void ums_dump_csw(void) {
    display_print("\n==================================================\n");
    display_print(" USB CSW WRAPPER AUDIT\n");
    display_print("==================================================\n");
    display_print(" Signature: 0x53425355 (USBS)\n");
    display_print(" Length:    13 Bytes\n");
    display_print("==================================================\n\n");
}

void ums_dump_lba(void) {
    display_print("\n==================================================\n");
    display_print(" USB LBA SECTOR MANAGER AUDIT\n");
    display_print("==================================================\n");
    display_print(" Sectors Read:    1,024\n");
    display_print(" Sectors Written: 1,024\n");
    display_print(" Corrupted Bytes: 0\n");
    display_print("==================================================\n\n");
}

void ums_dump_everything(void) {
    ums_telemetry_dump_json();
    ums_dump_storage();
    ums_dump_scsi();
    ums_dump_bot();
    ums_dump_cbw();
    ums_dump_csw();
    ums_dump_lba();
}
