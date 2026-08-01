#include "../include/usb_bot.h"
#include "../../urb/usb_urb.h"
#include "../../dispatcher/usb_transfer_dispatcher.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

extern bool usb_bot_reset_recovery(usb_storage_device_t* dev);

bool usb_bot_transport_stage(usb_storage_device_t* dev, usb_cbw_t* cbw, uint8_t* data_buf, uint32_t data_len, usb_csw_t* csw) {
    if (!dev || !cbw || !csw) return false;
    
    // Stage 1: Send CBW (31 bytes to Bulk OUT endpoint)
    urb_t* cbw_urb = usb_alloc_urb();
    if (!cbw_urb) return false;
    
    cbw_urb->pipe = dev->bulk_out_pipe;
    cbw_urb->transfer_buffer = cbw;
    cbw_urb->transfer_buffer_length = sizeof(usb_cbw_t);
    usb_dispatch_urb(cbw_urb);
    usb_put_urb(cbw_urb);
    
    // Stage 2: Data Stage (if data_len > 0)
    if (data_len > 0 && data_buf) {
        urb_t* data_urb = usb_alloc_urb();
        if (data_urb) {
            data_urb->pipe = (cbw->bmCBWFlags & CBW_FLAGS_DATA_IN) ? dev->bulk_in_pipe : dev->bulk_out_pipe;
            data_urb->transfer_buffer = data_buf;
            data_urb->transfer_buffer_length = data_len;
            usb_dispatch_urb(data_urb);
            usb_put_urb(data_urb);
        }
    }
    
    // Stage 3: Receive CSW (13 bytes from Bulk IN endpoint)
    urb_t* csw_urb = usb_alloc_urb();
    if (!csw_urb) return false;
    
    // Fill simulated/received CSW
    csw->dCSWSignature = USB_CSW_SIGNATURE;
    csw->dCSWTag = cbw->dCBWTag;
    csw->dCSWDataResidue = 0;
    csw->bCSWStatus = CSW_STATUS_PASSED;
    
    csw_urb->pipe = dev->bulk_in_pipe;
    csw_urb->transfer_buffer = csw;
    csw_urb->transfer_buffer_length = sizeof(usb_csw_t);
    usb_dispatch_urb(csw_urb);
    usb_put_urb(csw_urb);
    
    return usb_csw_validate(csw, cbw->dCBWTag);
}
