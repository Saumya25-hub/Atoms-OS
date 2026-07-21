#include "audio_driver_registry.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/audio/diagnostics/audio_debug.h"

#include "kernel/core/pci/pci.h"

static audio_hal_driver_t* g_registered_drivers[MAX_AUDIO_DRIVERS];
static uint32_t g_num_drivers = 0;
static audio_hal_driver_t* g_active_driver = NULL;

void audio_driver_registry_init(void) {
    g_num_drivers = 0;
    g_active_driver = NULL;
}

void audio_driver_registry_register(audio_hal_driver_t* driver) {
    if (!driver) return;
    if (g_num_drivers < MAX_AUDIO_DRIVERS) {
        g_registered_drivers[g_num_drivers++] = driver;
    }
}

audio_hal_driver_t* audio_driver_registry_discover_active(void) {
    if (g_active_driver != NULL) return g_active_driver;
    
    uint8_t target_bus = 0;
    uint8_t target_slot = 0;
    bool found = false;
    uint32_t vendor_device = 0;
    
    PCIDevice dev;
    if (pci_find_by_class(0x04, 0x01, &dev) || pci_find_by_class(0x04, 0x03, &dev)) {
        target_bus = dev.bus;
        target_slot = dev.slot;
        vendor_device = ((uint32_t)dev.device_id << 16) | dev.vendor_id;
        found = true;
    }
    
    if (!found) {
        return NULL; // No hardware found
    }
    
    // Attempt to match registered drivers
    for (uint32_t i = 0; i < g_num_drivers; i++) {
        audio_hal_driver_t* drv = g_registered_drivers[i];
        if (drv && drv->init) {
            // Struct containing bus info
            uint32_t pci_info[3] = {target_bus, target_slot, vendor_device};
            
            // Allow the driver to probe/init itself using the PCI info
            if (drv->init(pci_info)) {
                g_active_driver = drv;
                return drv;
            }
        }
    }
    
    return NULL;
}
