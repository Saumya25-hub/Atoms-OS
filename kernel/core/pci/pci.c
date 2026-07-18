#include "pci.h"
#include "arch/x86_64/io/port_io.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static PCIDevice g_pci_devices[MAX_PCI_DEVICES];
static uint32_t g_pci_device_count = 0;

uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    io_out32(PCI_CONFIG_ADDRESS, address);
    return io_in32(PCI_CONFIG_DATA);
}

void pci_write_config_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
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

void pci_init(void) {
    g_pci_device_count = 0;
    
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vd = pci_read_config((uint8_t)bus, slot, 0, 0);
            if (vd != 0xFFFFFFFF) {
                uint32_t class_code = pci_read_config((uint8_t)bus, slot, 0, 0x08);
                
                if (g_pci_device_count < MAX_PCI_DEVICES) {
                    g_pci_devices[g_pci_device_count].bus = (uint8_t)bus;
                    g_pci_devices[g_pci_device_count].slot = slot;
                    g_pci_devices[g_pci_device_count].func = 0;
                    g_pci_devices[g_pci_device_count].vendor_device = vd;
                    g_pci_devices[g_pci_device_count].base_class = (class_code >> 24) & 0xFF;
                    g_pci_devices[g_pci_device_count].sub_class = (class_code >> 16) & 0xFF;
                    g_pci_device_count++;
                }
            }
        }
    }
}

uint32_t pci_get_device_count(void) {
    return g_pci_device_count;
}

PCIDevice* pci_get_device(uint32_t index) {
    if (index >= g_pci_device_count) return NULL;
    return &g_pci_devices[index];
}

bool pci_find_by_class(uint8_t base_class, uint8_t sub_class, PCIDevice* out_device) {
    if (!out_device) return false;
    for (uint32_t i = 0; i < g_pci_device_count; i++) {
        if (g_pci_devices[i].base_class == base_class && g_pci_devices[i].sub_class == sub_class) {
            *out_device = g_pci_devices[i];
            return true;
        }
    }
    return false;
}
