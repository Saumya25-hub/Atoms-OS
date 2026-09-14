#include "audio_driver_registry.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/audio/diagnostics/audio_debug.h"
#include "kernel/drivers/display/display.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

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
    
    display_print("[BOS-AUDIO] PCI audio devices scanning\n");
    PCIDevice dev;
    
    /* 1. Probe for Intel High Definition Audio (Class 0x04, Subclass 0x03) */
    if (pci_find_by_class(0x04, 0x03, &dev)) {
        for (uint32_t i = 0; i < g_num_drivers; i++) {
            audio_hal_driver_t* drv = g_registered_drivers[i];
            if (drv && drv->init) {
                uint32_t pci_info[3] = {dev.bus, dev.slot, ((uint32_t)dev.device_id << 16) | dev.vendor_id};
                if (drv->init(pci_info)) {
                    g_active_driver = drv;
                    return drv;
                }
            }
        }
    }

    /* 2. Probe for Legacy AC97 Audio (Class 0x04, Subclass 0x01) */
    if (pci_find_by_class(0x04, 0x01, &dev)) {
        for (uint32_t i = 0; i < g_num_drivers; i++) {
            audio_hal_driver_t* drv = g_registered_drivers[i];
            if (drv && drv->init) {
                uint32_t pci_info[3] = {dev.bus, dev.slot, ((uint32_t)dev.device_id << 16) | dev.vendor_id};
                if (drv->init(pci_info)) {
                    g_active_driver = drv;
                    return drv;
                }
            }
        }
    }
    
    return NULL;
}
