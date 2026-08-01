#ifndef SIGNATURES_USB_PIPE_H
#define SIGNATURES_USB_PIPE_H

#include "../common/usb_common.h"
#include "../core/usb_core.h"
#include "../endpoint/usb_endpoint.h"
#include "kernel/core/sync/spinlock.h"

#define MAX_GLOBAL_PIPES 128

typedef enum {
    USB_PIPE_STATE_CLOSED = 0,
    USB_PIPE_STATE_ACTIVE,
    USB_PIPE_STATE_PAUSED,
    USB_PIPE_STATE_STALLED,
    USB_PIPE_STATE_SHUTDOWN,
    USB_PIPE_STATE_ERROR
} usb_pipe_state_t;

typedef struct {
    uint32_t pipe_handle;
    usb_device_t* dev;
    usb_endpoint_t* ep;
    usb_transfer_type_t type;
    usb_direction_t dir;
    usb_pipe_state_t state;
    
    uint16_t max_packet_size;
    uint32_t allocated_bandwidth;
    uint32_t total_urbs_processed;
    
    atoms_spinlock_t lock;
} usb_pipe_t;

// Pipe Manager APIs
void usb_pipe_manager_init(void);

usb_pipe_t* usb_create_pipe(usb_device_t* dev, uint8_t ep_num, usb_direction_t dir, usb_transfer_type_t type, uint16_t max_packet_size);
usb_pipe_t* usb_get_pipe_by_handle(uint32_t handle);

bool usb_pipe_reset(usb_pipe_t* pipe);
bool usb_pipe_pause(usb_pipe_t* pipe);
bool usb_pipe_resume(usb_pipe_t* pipe);
bool usb_pipe_shutdown(usb_pipe_t* pipe);
void usb_pipe_free(usb_pipe_t* pipe);

uint32_t usb_pipe_make_handle(uint8_t dev_addr, uint8_t ep_num, usb_direction_t dir, usb_transfer_type_t type);

#endif // SIGNATURES_USB_PIPE_H
