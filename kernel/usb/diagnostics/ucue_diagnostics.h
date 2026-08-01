#ifndef SIGNATURES_UCUE_DIAGNOSTICS_H
#define SIGNATURES_UCUE_DIAGNOSTICS_H

#include "../common/usb_common.h"
#include "../core/usb_core.h"
#include "../urb/usb_urb.h"
#include "../endpoint/usb_endpoint.h"
#include "../pipe/usb_pipe.h"
#include "../request/usb_request_queue.h"
#include "../dispatcher/usb_transfer_dispatcher.h"
#include "../resource/usb_resource_manager.h"

// Forensic Dump APIs
void ucue_dump_devices(void);
void ucue_dump_urbs(void);
void ucue_dump_endpoints(void);
void ucue_dump_pipes(void);
void ucue_dump_scheduler(void);
void ucue_dump_resources(void);
void ucue_dump_dispatcher(void);
void ucue_dump_everything(void);

// Telemetry Export API
void ucue_telemetry_init(void);
void ucue_telemetry_dump_json(void);

#endif // SIGNATURES_UCUE_DIAGNOSTICS_H
