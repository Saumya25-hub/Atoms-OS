#include "audio_driver_registry.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/audio/diagnostics/audio_debug.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static audio_hal_driver_t* g_registered_drivers[MAX_AUDIO_DRIVERS];
static uint32_t g_num_drivers = 0;
static audio_hal_driver_t* g_active_driver = NULL;

uint32_t audio_pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    io_out32(PCI_CONFIG_ADDRESS, address);
    return io_in32(PCI_CONFIG_DATA);
}

void audio_pci_write_config_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    io_out32(PCI_CONFIG_ADDRESS, address);
    
    uint32_t current = io_in32(PCI_CONFIG_DATA);
    
    if ((offset & 2) == 0) {
        current = (current & 0xFFFF0000) | value;
    } else {
        current = (current & 0x0000FFFF) | ((uint32_t)value << 16);
    }
    
    io_out32(PCI_CONFIG_DATA, current);
}

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
    
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vd = audio_pci_read_config((uint8_t)bus, slot, 0, 0);
            if (vd != 0xFFFFFFFF) {
                uint32_t class_code = audio_pci_read_config((uint8_t)bus, slot, 0, 0x08);
                uint8_t base_class = (class_code >> 24) & 0xFF;
                uint8_t sub_class = (class_code >> 16) & 0xFF;
                
                if (base_class == 0x04 && (sub_class == 0x01 || sub_class == 0x03)) {
                    target_bus = (uint8_t)bus;
                    target_slot = slot;
                    vendor_device = vd;
                    found = true;
                    break;
                }
            }
        }
        if (found) break;
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
