#ifndef SIGNATURES_USB_HUB_TOPOLOGY_H
#define SIGNATURES_USB_HUB_TOPOLOGY_H

#include "../../common/usb_common.h"

#define USB_MAX_HUB_DEPTH 7
#define USB_MAX_PATH_LEN  32

typedef struct usb_topology_node {
    uint32_t                    node_id;
    bool                        is_hub;
    uint32_t                    parent_hub_id;
    uint8_t                     parent_port_num;
    uint32_t                    controller_id;
    usb_speed_t                 speed;
    uint8_t                     depth;
    char                        path[USB_MAX_PATH_LEN]; // e.g. "1.2.4"
    struct usb_topology_node*   children[16];
    uint32_t                    child_count;
    atoms_spinlock_t            lock;
} usb_topology_node_t;

// API
void usb_topology_init(void);
usb_topology_node_t* usb_topology_create_node(uint32_t node_id, bool is_hub, uint32_t parent_hub_id, uint8_t parent_port_num, uint32_t controller_id, usb_speed_t speed, uint8_t depth);
bool usb_topology_attach_child(uint32_t parent_hub_id, usb_topology_node_t* child);
bool usb_topology_detach_child(uint32_t parent_hub_id, uint32_t child_id);
usb_topology_node_t* usb_topology_find_node(uint32_t node_id);
bool usb_topology_validate_depth(uint8_t depth);

#endif // SIGNATURES_USB_HUB_TOPOLOGY_H
