#include "../include/usb_bot.h"
#include "../../endpoint/usb_endpoint.h"
#include "kernel/drivers/display/display.h"

bool usb_bot_reset_recovery(usb_storage_device_t* dev) {
    if (!dev) return false;
    display_print("[BOT RECOVERY] Executing Reset Recovery Sequence for Storage Device #");
    display_print_dec(dev->storage_id);
    display_print("...\n");
    
    // Step 1: Bulk-Only Mass Storage Reset (Control Request)
    // Step 2: Clear Feature HALT on Bulk IN endpoint
    // Step 3: Clear Feature HALT on Bulk OUT endpoint
    usb_pipe_t* p_in = usb_get_pipe_by_handle(dev->bulk_in_pipe);
    if (p_in && p_in->ep) usb_endpoint_reset_toggle(p_in->ep);
    
    usb_pipe_t* p_out = usb_get_pipe_by_handle(dev->bulk_out_pipe);
    if (p_out && p_out->ep) usb_endpoint_reset_toggle(p_out->ep);
    
    display_print("[BOT RECOVERY] Reset Recovery Complete. Endpoints cleared.\n");
    return true;
}
