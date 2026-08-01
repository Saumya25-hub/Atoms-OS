#ifndef SIGNATURES_USB_CONTROLLER_MANAGER_H
#define SIGNATURES_USB_CONTROLLER_MANAGER_H

#include "../common/usb_common.h"
#include "kernel/core/pci/pci.h"
#include "kernel/core/sync/spinlock.h"

#define MAX_USB_CONTROLLERS 16

typedef struct usb_controller_ops {
    bool (*init)(void* ctrl_ctx);
    void (*shutdown)(void* ctrl_ctx);
    bool (*reset)(void* ctrl_ctx);
    bool (*port_reset)(void* ctrl_ctx, uint8_t port);
    bool (*submit_transfer)(void* ctrl_ctx, void* req);
    void (*poll)(void* ctrl_ctx);
} usb_controller_ops_t;

typedef struct {
    uint32_t id;
    usb_controller_type_t type;
    const char* name;
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint8_t irq;
    uint64_t mmio_base;
    uint16_t io_base;
    bool active;
    bool healthy;
    uint32_t port_count;
    void* private_data;
    usb_controller_ops_t ops;
} usb_controller_device_t;

typedef struct {
    usb_controller_device_t controllers[MAX_USB_CONTROLLERS];
    uint32_t count;
    uint32_t uhci_count;
    uint32_t ohci_count;
    uint32_t ehci_count;
    uint32_t xhci_count;
    atoms_spinlock_t lock;
} usb_controller_manager_t;

// Controller Manager APIs
void usb_controller_manager_init(void);
uint32_t usb_controller_scan_pci(void);
bool usb_controller_register(usb_controller_type_t type, PCIDevice* pci_dev, const char* name, void* private_data, usb_controller_ops_t ops);
bool usb_controller_register_manual(usb_controller_type_t type, const char* name, void* private_data, uint16_t io_base, uint64_t mmio_base, uint8_t irq, uint32_t port_count);
usb_controller_device_t* usb_controller_get(uint32_t index);
uint32_t usb_controller_count(void);
usb_controller_manager_t* usb_get_controller_manager(void);

#endif // SIGNATURES_USB_CONTROLLER_MANAGER_H
