#include "../include/usb_hub_port.h"
#include "kernel/drivers/display/display.h"

const char* usb_port_state_to_string(usb_port_state_t state) {
    switch (state) {
        case PORT_STATE_DISCONNECTED: return "DISCONNECTED";
        case PORT_STATE_CONNECTED:    return "CONNECTED";
        case PORT_STATE_POWERED:      return "POWERED";
        case PORT_STATE_RESETTING:    return "RESETTING";
        case PORT_STATE_ENUMERATING:  return "ENUMERATING";
        case PORT_STATE_CONFIGURED:   return "CONFIGURED";
        case PORT_STATE_READY:        return "READY";
        case PORT_STATE_SUSPENDED:    return "SUSPENDED";
        case PORT_STATE_RESUMED:      return "RESUMED";
        case PORT_STATE_REMOVED:      return "REMOVED";
        case PORT_STATE_ERROR:        return "ERROR";
        default: return "UNKNOWN";
    }
}

void usb_port_init(usb_hub_port_t* port, uint8_t port_num) {
    if (!port) return;
    port->port_num = port_num;
    port->state = PORT_STATE_DISCONNECTED;
    port->status = 0;
    port->change = 0;
    port->speed = USB_SPEED_LOW;
    port->power_ma = 0;
    port->is_overcurrent = false;
    port->child_device_id = 0;
}
