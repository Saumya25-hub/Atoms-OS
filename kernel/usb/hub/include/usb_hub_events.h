#ifndef SIGNATURES_USB_HUB_EVENTS_H
#define SIGNATURES_USB_HUB_EVENTS_H

#include "../../common/usb_common.h"
#include "usb_hub_port.h"

typedef enum {
    HUB_EVENT_NONE = 0,
    HUB_EVENT_HOT_PLUG,
    HUB_EVENT_HOT_REMOVE,
    HUB_EVENT_OVERCURRENT,
    HUB_EVENT_PORT_ERROR,
    HUB_EVENT_RESET_COMPLETE
} usb_hub_event_type_t;

typedef struct {
    usb_hub_event_type_t type;
    uint32_t             hub_id;
    uint8_t              port_num;
    usb_speed_t          speed;
    uint32_t             timestamp;
} usb_hub_event_t;

#define USB_MAX_HUB_EVENTS 64

typedef struct {
    usb_hub_event_t  queue[USB_MAX_HUB_EVENTS];
    uint32_t         head;
    uint32_t         tail;
    uint32_t         count;
    uint32_t         total_processed;
    atoms_spinlock_t lock;
} usb_hub_event_queue_t;

// API
void usb_hub_events_init(void);
bool usb_hub_enqueue_event(usb_hub_event_type_t type, uint32_t hub_id, uint8_t port_num, usb_speed_t speed);
bool usb_hub_dequeue_event(usb_hub_event_t* out_event);
void usb_hub_process_events(void);

#endif // SIGNATURES_USB_HUB_EVENTS_H
