#include "usb_controller_manager.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

static usb_controller_manager_t g_ctrl_mgr;

void usb_controller_manager_init(void) {
    memset(&g_ctrl_mgr, 0, sizeof(usb_controller_manager_t));
    atoms_spinlock_init(&g_ctrl_mgr.lock, 1);
    display_print("[USB MANAGER] Master USB Controller Manager Initialized.\n");
}

uint32_t usb_controller_scan_pci(void) {
    atoms_irq_lock_state_t lock_state = atoms_spin_lock_irqsave(&g_ctrl_mgr.lock);
    
    uint32_t count = pci_get_device_count();
    uint32_t detected = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        PCIDevice* dev = pci_get_device(i);
        if (!dev) continue;
        
        // USB Base Class = 0x0C, Subclass = 0x03
        if (dev->base_class == 0x0C && dev->sub_class == 0x03) {
            uint32_t class_info = pci_read_config(dev->bus, dev->slot, dev->func, 0x08);
            uint8_t prog_if = (class_info >> 8) & 0xFF;
            
            usb_controller_type_t type = USB_CONTROLLER_TYPE_UNKNOWN;
            const char* type_str = "UNKNOWN";
            
            if (prog_if == 0x00) {
                type = USB_CONTROLLER_TYPE_UHCI;
                type_str = "UHCI (USB 1.1 Intel)";
                g_ctrl_mgr.uhci_count++;
            } else if (prog_if == 0x10) {
                type = USB_CONTROLLER_TYPE_OHCI;
                type_str = "OHCI (USB 1.1 AMD/VIA)";
                g_ctrl_mgr.ohci_count++;
            } else if (prog_if == 0x20) {
                type = USB_CONTROLLER_TYPE_EHCI;
                type_str = "EHCI (USB 2.0 High-Speed)";
                g_ctrl_mgr.ehci_count++;
            } else if (prog_if == 0x30) {
                type = USB_CONTROLLER_TYPE_XHCI;
                type_str = "xHCI (USB 3.x SuperSpeed)";
                g_ctrl_mgr.xhci_count++;
            }
            
            if (type != USB_CONTROLLER_TYPE_UNKNOWN && g_ctrl_mgr.count < MAX_USB_CONTROLLERS) {
                usb_controller_device_t* ctrl = &g_ctrl_mgr.controllers[g_ctrl_mgr.count++];
                ctrl->id = g_ctrl_mgr.count - 1;
                ctrl->type = type;
                ctrl->name = type_str;
                ctrl->bus = dev->bus;
                ctrl->slot = dev->slot;
                ctrl->func = dev->func;
                ctrl->irq = dev->interrupt_line;
                ctrl->active = true;
                ctrl->healthy = true;
                
                // Read BAR0 base address
                uint32_t bar0 = pci_read_config(dev->bus, dev->slot, dev->func, 0x10);
                if (bar0 & 1) {
                    ctrl->io_base = (uint16_t)(bar0 & ~0x3U);
                } else {
                    ctrl->mmio_base = bar0 & ~0xFU;
                }
                
                detected++;
                display_print("[USB PCI DETECT] ");
                display_print(type_str);
                display_print(" at ");
                display_print_dec(dev->bus); display_print(":");
                display_print_dec(dev->slot); display_print(".");
                display_print_dec(dev->func); display_print(" IRQ=");
                display_print_dec(dev->interrupt_line); display_print("\n");
            }
        }
    }
    
    atoms_spin_unlock_irqrestore(&g_ctrl_mgr.lock, lock_state);
    return detected;
}

bool usb_controller_register(usb_controller_type_t type, PCIDevice* pci_dev, const char* name, void* private_data, usb_controller_ops_t ops) {
    if (!pci_dev || !name) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_ctrl_mgr.lock);
    
    if (g_ctrl_mgr.count >= MAX_USB_CONTROLLERS) {
        atoms_spin_unlock_irqrestore(&g_ctrl_mgr.lock, state);
        return false;
    }
    
    usb_controller_device_t* ctrl = &g_ctrl_mgr.controllers[g_ctrl_mgr.count++];
    ctrl->id = g_ctrl_mgr.count - 1;
    ctrl->type = type;
    ctrl->name = name;
    ctrl->bus = pci_dev->bus;
    ctrl->slot = pci_dev->slot;
    ctrl->func = pci_dev->func;
    ctrl->irq = pci_dev->interrupt_line;
    ctrl->active = true;
    ctrl->healthy = true;
    ctrl->private_data = private_data;
    ctrl->ops = ops;
    
    if (type == USB_CONTROLLER_TYPE_UHCI) g_ctrl_mgr.uhci_count++;
    else if (type == USB_CONTROLLER_TYPE_OHCI) g_ctrl_mgr.ohci_count++;
    else if (type == USB_CONTROLLER_TYPE_EHCI) g_ctrl_mgr.ehci_count++;
    else if (type == USB_CONTROLLER_TYPE_XHCI) g_ctrl_mgr.xhci_count++;

    atoms_spin_unlock_irqrestore(&g_ctrl_mgr.lock, state);
    return true;
}

bool usb_controller_register_manual(usb_controller_type_t type, const char* name, void* private_data, uint16_t io_base, uint64_t mmio_base, uint8_t irq, uint32_t port_count) {
    if (!name) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_ctrl_mgr.lock);
    
    if (g_ctrl_mgr.count >= MAX_USB_CONTROLLERS) {
        atoms_spin_unlock_irqrestore(&g_ctrl_mgr.lock, state);
        return false;
    }
    
    usb_controller_device_t* ctrl = &g_ctrl_mgr.controllers[g_ctrl_mgr.count++];
    ctrl->id = g_ctrl_mgr.count - 1;
    ctrl->type = type;
    ctrl->name = name;
    ctrl->io_base = io_base;
    ctrl->mmio_base = mmio_base;
    ctrl->irq = irq;
    ctrl->port_count = port_count;
    ctrl->active = true;
    ctrl->healthy = true;
    ctrl->private_data = private_data;

    if (type == USB_CONTROLLER_TYPE_UHCI) g_ctrl_mgr.uhci_count++;
    else if (type == USB_CONTROLLER_TYPE_OHCI) g_ctrl_mgr.ohci_count++;
    else if (type == USB_CONTROLLER_TYPE_EHCI) g_ctrl_mgr.ehci_count++;
    else if (type == USB_CONTROLLER_TYPE_XHCI) g_ctrl_mgr.xhci_count++;
    
    atoms_spin_unlock_irqrestore(&g_ctrl_mgr.lock, state);
    return true;
}

usb_controller_device_t* usb_controller_get(uint32_t index) {
    if (index >= g_ctrl_mgr.count) return NULL;
    return &g_ctrl_mgr.controllers[index];
}

uint32_t usb_controller_count(void) {
    return g_ctrl_mgr.count;
}

usb_controller_manager_t* usb_get_controller_manager(void) {
    return &g_ctrl_mgr;
}
