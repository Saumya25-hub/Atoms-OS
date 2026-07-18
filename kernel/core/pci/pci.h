#ifndef SIGNATURES_PCI_H
#define SIGNATURES_PCI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_PCI_DEVICES 64

typedef struct {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint32_t vendor_device; // Vendor ID in low 16 bits, Device ID in high 16 bits
    uint8_t base_class;
    uint8_t sub_class;
} PCIDevice;

void pci_init(void);
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write_config_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);
uint32_t pci_get_device_count(void);
PCIDevice* pci_get_device(uint32_t index);
bool pci_find_by_class(uint8_t base_class, uint8_t sub_class, PCIDevice* out_device);

#endif // SIGNATURES_PCI_H
