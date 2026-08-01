#ifndef SIGNATURES_USB_HUB_PORT_H
#define SIGNATURES_USB_HUB_PORT_H

#include "../../common/usb_common.h"

// 11-Stage Port State Machine
typedef enum {
    PORT_STATE_DISCONNECTED = 0,
    PORT_STATE_CONNECTED,
    PORT_STATE_POWERED,
    PORT_STATE_RESETTING,
    PORT_STATE_ENUMERATING,
    PORT_STATE_CONFIGURED,
    PORT_STATE_READY,
    PORT_STATE_SUSPENDED,
    PORT_STATE_RESUMED,
    PORT_STATE_REMOVED,
    PORT_STATE_ERROR
} usb_port_state_t;

// Port Status Bits (USB Spec 2.0 Table 11-21)
#define PORT_STAT_CONNECTION        0x0001
#define PORT_STAT_ENABLE            0x0002
#define PORT_STAT_SUSPEND           0x0004
#define PORT_STAT_OVERCURRENT       0x0008
#define PORT_STAT_RESET             0x0010
#define PORT_STAT_POWER             0x0100
#define PORT_STAT_LOW_SPEED         0x0200
#define PORT_STAT_HIGH_SPEED        0x0400
#define PORT_STAT_TEST              0x0800
#define PORT_STAT_INDICATOR         0x1000

// Port Change Bits (USB Spec 2.0 Table 11-22)
#define PORT_STAT_C_CONNECTION      0x0001
#define PORT_STAT_C_ENABLE          0x0002
#define PORT_STAT_C_SUSPEND         0x0004
#define PORT_STAT_C_OVERCURRENT     0x0008
#define PORT_STAT_C_RESET           0x0010

// Port Feature Selectors
#define PORT_FEATURE_CONNECTION     0
#define PORT_FEATURE_ENABLE         1
#define PORT_FEATURE_SUSPEND        2
#define PORT_FEATURE_OVER_CURRENT   3
#define PORT_FEATURE_RESET          4
#define PORT_FEATURE_POWER          8
#define PORT_FEATURE_LOWSPEED       9
#define PORT_FEATURE_C_CONNECTION   16
#define PORT_FEATURE_C_ENABLE       17
#define PORT_FEATURE_C_SUSPEND      18
#define PORT_FEATURE_C_OVER_CURRENT  19
#define PORT_FEATURE_C_RESET        20

// Port Structure
typedef struct {
    uint8_t             port_num;       // 1-indexed
    usb_port_state_t    state;
    uint16_t            status;
    uint16_t            change;
    usb_speed_t         speed;
    uint32_t            power_ma;       // Allocated power budget in mA
    bool                is_overcurrent;
    uint32_t            child_device_id; // Registered child device ID (if any)
    atoms_spinlock_t    lock;
} usb_hub_port_t;

// API
const char* usb_port_state_to_string(usb_port_state_t state);
void usb_port_init(usb_hub_port_t* port, uint8_t port_num);

#endif // SIGNATURES_USB_HUB_PORT_H
