#include "usb_core.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_core_registry_t g_usb_core;

void ucue_usb_core_init(void) {
    memset(&g_usb_core, 0, sizeof(usb_core_registry_t));
    atoms_spinlock_init(&g_usb_core.lock, 1);
    g_usb_core.next_address = 1;
    g_usb_core.initialized = true;
    display_print("[USB CORE] Subsystem Initialized. Unified Core Ready.\n");
}

bool usb_register_driver(usb_driver_t* driver) {
    if (!driver || !driver->name) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_usb_core.lock);
    
    if (g_usb_core.driver_count >= MAX_USB_DRIVERS) {
        atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
        return false;
    }
    
    for (uint32_t i = 0; i < g_usb_core.driver_count; i++) {
        if (g_usb_core.drivers[i].name && strcmp(g_usb_core.drivers[i].name, driver->name) == 0) {
            atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
            return false; // Already registered
        }
    }
    
    g_usb_core.drivers[g_usb_core.driver_count++] = *driver;
    display_print("[USB CORE] Registered Driver: ");
    display_print(driver->name);
    display_print("\n");
    
    atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
    return true;
}

bool usb_unregister_driver(usb_driver_t* driver) {
    if (!driver || !driver->name) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_usb_core.lock);
    
    for (uint32_t i = 0; i < g_usb_core.driver_count; i++) {
        if (g_usb_core.drivers[i].name && strcmp(g_usb_core.drivers[i].name, driver->name) == 0) {
            for (uint32_t j = i; j < g_usb_core.driver_count - 1; j++) {
                g_usb_core.drivers[j] = g_usb_core.drivers[j + 1];
            }
            g_usb_core.driver_count--;
            atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
            return true;
        }
    }
    
    atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
    return false;
}

usb_device_t* ucue_usb_register_device(uint32_t controller_id, usb_controller_type_t ctrl_type, uint8_t port, usb_speed_t speed) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_usb_core.lock);
    
    if (g_usb_core.device_count >= MAX_USB_DEVICES) {
        atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
        return NULL;
    }
    
    usb_device_t* dev = &g_usb_core.devices[g_usb_core.device_count++];
    memset(dev, 0, sizeof(usb_device_t));
    dev->device_id = g_usb_core.device_count;
    dev->address = (g_usb_core.next_address <= 127) ? g_usb_core.next_address++ : 1;
    dev->port_num = port;
    dev->speed = speed;
    dev->controller_id = controller_id;
    dev->controller_type = ctrl_type;
    dev->state = USB_DEV_STATE_ATTACHED;
    dev->ref_count = 1;
    dev->max_packet_size0 = (speed == USB_SPEED_HIGH || speed == USB_SPEED_SUPER) ? 64 : 8;
    atoms_spinlock_init(&dev->lock, 1);
    
    // Transition to Addressed and Configured state
    dev->state = USB_DEV_STATE_ADDRESSED;
    dev->active_config = 1;
    dev->active_interface = 0;
    dev->state = USB_DEV_STATE_CONFIGURED;
    
    display_print("[USB CORE] Registered Device #");
    display_print_dec(dev->device_id);
    display_print(" Address ");
    display_print_dec(dev->address);
    display_print(" (Speed: ");
    display_print_dec((uint32_t)speed);
    display_print(")\n");
    
    atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
    return dev;
}

bool usb_unregister_device(usb_device_t* dev) {
    if (!dev) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_usb_core.lock);
    
    for (uint32_t i = 0; i < g_usb_core.device_count; i++) {
        if (&g_usb_core.devices[i] == dev) {
            dev->state = USB_DEV_STATE_DISCONNECTED;
            for (uint32_t j = i; j < g_usb_core.device_count - 1; j++) {
                g_usb_core.devices[j] = g_usb_core.devices[j + 1];
            }
            g_usb_core.device_count--;
            atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
            return true;
        }
    }
    
    atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
    return false;
}

usb_device_t* usb_find_device(uint32_t device_id) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_usb_core.lock);
    for (uint32_t i = 0; i < g_usb_core.device_count; i++) {
        if (g_usb_core.devices[i].device_id == device_id) {
            atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
            return &g_usb_core.devices[i];
        }
    }
    atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
    return NULL;
}

usb_device_t* usb_get_device_by_address(uint8_t address) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_usb_core.lock);
    for (uint32_t i = 0; i < g_usb_core.device_count; i++) {
        if (g_usb_core.devices[i].address == address) {
            atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
            return &g_usb_core.devices[i];
        }
    }
    atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
    return NULL;
}

usb_device_t* ucue_usb_get_device_by_slot(uint8_t slot_id) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_usb_core.lock);
    for (uint32_t i = 0; i < g_usb_core.device_count; i++) {
        if (g_usb_core.devices[i].port_num == slot_id || g_usb_core.devices[i].address == slot_id || g_usb_core.devices[i].device_id == slot_id) {
            atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
            return &g_usb_core.devices[i];
        }
    }
    atoms_spin_unlock_irqrestore(&g_usb_core.lock, state);
    return NULL;
}

void usb_get_device(usb_device_t* dev) {
    if (!dev) return;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&dev->lock);
    dev->ref_count++;
    atoms_spin_unlock_irqrestore(&dev->lock, state);
}

void usb_put_device(usb_device_t* dev) {
    if (!dev) return;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&dev->lock);
    if (dev->ref_count > 0) dev->ref_count--;
    atoms_spin_unlock_irqrestore(&dev->lock, state);
}

bool usb_set_configuration(usb_device_t* dev, uint8_t config_val) {
    if (!dev) return false;
    dev->active_config = config_val;
    dev->state = USB_DEV_STATE_CONFIGURED;
    return true;
}

uint8_t usb_get_configuration(usb_device_t* dev) {
    return dev ? (uint8_t)dev->active_config : 0;
}

bool usb_set_interface(usb_device_t* dev, uint8_t ifnum, uint8_t altsetting) {
    if (!dev) return false;
    dev->active_interface = ifnum;
    (void)altsetting;
    return true;
}

uint8_t usb_get_interface(usb_device_t* dev) {
    return dev ? (uint8_t)dev->active_interface : 0;
}

bool usb_reset_device(usb_device_t* dev) {
    if (!dev) return false;
    dev->state = USB_DEV_STATE_RECOVERY;
    display_print("[USB CORE] Resetting Device #");
    display_print_dec(dev->device_id);
    display_print("\n");
    dev->state = USB_DEV_STATE_CONFIGURED;
    return true;
}

bool usb_suspend_device(usb_device_t* dev) {
    if (!dev) return false;
    dev->state = USB_DEV_STATE_SUSPENDED;
    display_print("[USB CORE] Device #");
    display_print_dec(dev->device_id);
    display_print(" Suspended\n");
    return true;
}

bool usb_resume_device(usb_device_t* dev) {
    if (!dev) return false;
    dev->state = USB_DEV_STATE_RESUMED;
    dev->state = USB_DEV_STATE_CONFIGURED;
    display_print("[USB CORE] Device #");
    display_print_dec(dev->device_id);
    display_print(" Resumed\n");
    return true;
}

void usb_disconnect_device(usb_device_t* dev) {
    if (!dev) return;
    dev->state = USB_DEV_STATE_DISCONNECTED;
    display_print("[USB CORE] Device #");
    display_print_dec(dev->device_id);
    display_print(" Disconnected\n");
}

usb_core_registry_t* usb_get_core_registry(void) {
    return &g_usb_core;
}
