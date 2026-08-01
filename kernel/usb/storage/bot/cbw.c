#include "../include/usb_cbw.h"
#include "kernel/core/lib/include/string.h"

void usb_cbw_init(usb_cbw_t* cbw, uint32_t tag, uint32_t transfer_length, uint8_t flags, uint8_t lun, uint8_t cdb_length, const uint8_t* cdb) {
    if (!cbw) return;
    memset(cbw, 0, sizeof(usb_cbw_t));
    cbw->dCBWSignature = USB_CBW_SIGNATURE;
    cbw->dCBWTag = tag;
    cbw->dCBWDataTransferLength = transfer_length;
    cbw->bmCBWFlags = flags;
    cbw->bCBWLUN = lun & 0x0F;
    cbw->bCBWCBLength = (cdb_length > 16) ? 16 : cdb_length;
    if (cdb && cdb_length > 0) {
        memcpy(cbw->CBWCB, cdb, cbw->bCBWCBLength);
    }
}
